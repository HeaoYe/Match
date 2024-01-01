#include <Match/vulkan/resource/renderpass_builder.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/setting.hpp>
#include <Match/constant.hpp>
#include "../inner.hpp"

namespace Match {
    vk::AttachmentReference SubpassBuilder::create_reference(const std::string &name, vk::ImageLayout layout, bool is_attachment_read) {
        return { builder.get_attachment_index(name, is_attachment_read), layout };
    }
    
    SubpassBuilder &SubpassBuilder::bind(vk::PipelineBindPoint bind_point) {
        this->bind_point = bind_point;
        return *this;
    }

    SubpassBuilder &SubpassBuilder::attach_input_attachment(const std::string &name, vk::ImageLayout layout) {
        input_attachments.push_back(create_reference(name, layout, true));
        return *this;
    }

    SubpassBuilder &SubpassBuilder::attach_output_attachment(const std::string &name, vk::ImageLayout layout) {
        output_attachments.push_back(create_reference(name, layout, false));
        output_attachment_blend_states.push_back({
            VK_FALSE,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::BlendFactor::eZero,
            vk::BlendFactor::eZero,
            vk::BlendOp::eAdd,
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,

        });
        resolve_attachments.push_back(create_reference(name, layout, true));
        return *this;
    }
    
    SubpassBuilder &SubpassBuilder::attach_preserve_attachment(const std::string &name) {
        uint32_t idx1 = builder.get_attachment_index(name, false);
        uint32_t idx2 = builder.get_attachment_index(name, true);
        preserve_attachments.push_back(idx1);
        if (idx1 != idx2) {
            preserve_attachments.push_back(idx2);
        }
        return *this;
    }
    
    SubpassBuilder &SubpassBuilder::attach_depth_attachment(const std::string &name, vk::ImageLayout layout) {
        depth_attachment = create_reference(name, layout, false);
        return *this;
    }

    SubpassBuilder &SubpassBuilder::set_output_attachment_color_blend(const std::string &name, vk::PipelineColorBlendAttachmentState state) {
        uint32_t i = 0, idx = builder.get_attachment_index(name, false);
        for (const auto &reference : output_attachments) {
            if (reference.attachment == idx) {
                output_attachment_blend_states[i] = state;
                break;
            }
            i ++;
        }
        return *this;
    }

    SubpassBuilder &SubpassBuilder::wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other) {
        uint32_t src_subpass;
        if (name == EXTERNAL_SUBPASS) {
            src_subpass = VK_SUBPASS_EXTERNAL;
        } else {
            src_subpass = builder.get_subpass_index(name);
        }
        builder.final_dependencies.push_back({
            src_subpass,
            builder.get_subpass_index(this->name),
            other.stage,
            self.stage,
            other.access,
            self.access,
        });
        return *this;
    }

    vk::SubpassDescription SubpassBuilder::build() const {
        vk::SubpassDescription desc {};
        desc.pipelineBindPoint = bind_point;
        desc.inputAttachmentCount = input_attachments.size();
        desc.pInputAttachments = input_attachments.data();
        desc.colorAttachmentCount = output_attachments.size();
        desc.pColorAttachments = output_attachments.data();
        if ((resolve_attachments.size() != 0) && (resolve_attachments.size() != output_attachments.size())) {
            MCH_ERROR("Subpass \"{}\": Resolve Attachment Count != Output Attachment Count", name)
        }
        if (runtime_setting->is_msaa_enabled()) {
            desc.pResolveAttachments = resolve_attachments.data();
        }
        if (depth_attachment.has_value()) {
            desc.pDepthStencilAttachment = &depth_attachment.value();
        }
        desc.preserveAttachmentCount = preserve_attachments.size();
        desc.pPreserveAttachments = preserve_attachments.data();
        return std::move(desc);
    }

    RenderPassBuilder::RenderPassBuilder() {
        add_attachment(SWAPCHAIN_IMAGE_ATTACHMENT, AttachmentType::eColor);
        auto &attachment = attachments[0];
        if (attachment.description_read.has_value()) {
            attachment.description_read->finalLayout = vk::ImageLayout::ePresentSrcKHR;
        } else {
            attachment.description_write.finalLayout = vk::ImageLayout::ePresentSrcKHR;
        }
    }

    RenderPassBuilder &RenderPassBuilder::add_attachment(const std::string &name, AttachmentType type) {
        attachments_map.insert(std::make_pair(name, attachments.size()));
        auto &attachment = attachments.emplace_back();
        attachment.description_write.setSamples(runtime_setting->multisample_count)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined);
        switch (type) {
        case AttachmentType::eColor:
            attachment.description_write.format = manager->swapchain->format.format;
            attachment.description_write.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
            attachment.usage = vk::ImageUsageFlagBits::eColorAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_value = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            if (runtime_setting->is_msaa_enabled()) {
                attachment.description_read = attachment.description_write;
                attachment.description_read->samples = vk::SampleCountFlagBits::e1;
                attachment.description_read->loadOp = vk::AttachmentLoadOp::eDontCare;
                attachment.usage_read = vk::ImageUsageFlagBits::eColorAttachment;
                attachment.offset = resolved_attachment_count;
                resolved_attachment_count += 1;
            }
            break;
        case AttachmentType::eDepthBuffer:
            attachment.usage = vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
        case AttachmentType::eDepth:
            attachment.description_write.format = get_supported_depth_format();
            attachment.description_write.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
            attachment.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eDepth;
            if (has_stencil_component(attachment.description_write.format)) {
                attachment.aspect |= vk::ImageAspectFlagBits::eStencil;
            }
            attachment.clear_value = { { 1.0f, 0 } };
            break;
        case AttachmentType::eFloat4Buffer:
            attachment.description_write.format = vk::Format::eR32G32B32A32Sfloat;
            attachment.description_write.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            attachment.usage = vk::ImageUsageFlagBits::eColorAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_value = { { 0.0f, 0.0f, 0.0f, 1.0f } };
            if (runtime_setting->is_msaa_enabled()) {
                attachment.description_read = attachment.description_write;
                attachment.description_read->samples = vk::SampleCountFlagBits::e1;
                attachment.description_read->loadOp = vk::AttachmentLoadOp::eDontCare;
                attachment.description_write.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
                attachment.usage_read = vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
                attachment.offset = resolved_attachment_count;
                resolved_attachment_count += 1;
            } else {
                attachment.usage |= vk::ImageUsageFlagBits::eInputAttachment | vk::ImageUsageFlagBits::eSampled;
            }
            break;
        }
        return *this;
    }

    SubpassBuilder &RenderPassBuilder::add_subpass(const std::string &name) {
        subpass_builders_map.insert(std::make_pair(name, subpass_builders.size()));
        subpass_builders.push_back(std::make_unique<SubpassBuilder>(name, *this));
        return *subpass_builders.back();
    }

    vk::RenderPassCreateInfo RenderPassBuilder::build() {
        vk::RenderPassCreateInfo create_info {};
        final_attachments.clear();
        final_attachments.reserve(attachments.size() + resolved_attachment_count);
        for (const auto &attachment : attachments) {
            final_attachments.push_back(attachment.description_write);
        }
        if (runtime_setting->is_msaa_enabled()) {
            for (const auto &attachment : attachments) {
                if (attachment.description_read.has_value()) {
                    final_attachments.push_back(attachment.description_read.value());
                }
            }
        }
        final_subpasses.clear();
        final_subpasses.reserve(subpass_builders.size());
        for (const auto &subpass_builder : subpass_builders) {
            final_subpasses.push_back(subpass_builder->build());
        }
        create_info.setAttachments(final_attachments)
            .setSubpasses(final_subpasses)
            .setDependencies(final_dependencies);
        return std::move(create_info);
    }

    uint32_t RenderPassBuilder::get_attachment_index(const std::string &name, bool is_attachment_read) {
        auto idx = attachments_map.find(name);
        if (idx == attachments_map.end()) {
            MCH_ERROR("No attachemnt named {}", name);
            return -1u;
        }
        if (is_attachment_read && attachments[idx->second].description_read.has_value()) {
            return attachments[idx->second].offset + attachments.size();
        }
        return idx->second;
    }

    uint32_t RenderPassBuilder::get_subpass_index(const std::string &name) {
        auto idx = subpass_builders_map.find(name);
        if (idx == subpass_builders_map.end()) {
            MCH_ERROR("No subpass named {}", name);
            return -1u;
        }
        return idx->second;
    }
}
