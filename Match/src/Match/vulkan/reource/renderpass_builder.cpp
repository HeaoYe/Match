#include <Match/vulkan/resource/renderpass_builder.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/setting.hpp>
#include <Match/constant.hpp>
#include "../inner.hpp"

namespace Match {
    VkAttachmentReference SubpassBuilder::create_reference(const std::string &name, VkImageLayout layout, bool is_attachment_read) {
        return { builder.get_attachment_index(name, is_attachment_read), layout };
    }
    
    void SubpassBuilder::bind(VkPipelineBindPoint bind_point) {
        this->bind_point = bind_point;
    }

    void SubpassBuilder::attach_input_attachment(const std::string &name, VkImageLayout layout) {
        input_attachments.push_back(create_reference(name, layout, true));
    }

    void SubpassBuilder::attach_output_attachment(const std::string &name, VkImageLayout layout) {
        output_attachments.push_back(create_reference(name, layout, false));
        output_attachment_blend_states.push_back({
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });
        resolve_attachments.push_back(create_reference(name, layout, true));
    }
    
    void SubpassBuilder::attach_preserve_attachment(const std::string &name) {
        uint32_t idx1 = builder.get_attachment_index(name, false);
        uint32_t idx2 = builder.get_attachment_index(name, true);
        preserve_attachments.push_back(idx1);
        if (idx1 != idx2) {
            preserve_attachments.push_back(idx2);
        }
    }
    
    void SubpassBuilder::attach_depth_attachment(const std::string &name, VkImageLayout layout) {
        depth_attachment = create_reference(name, layout, false);
    }

    void SubpassBuilder::set_output_attachment_color_blend(const std::string &name, VkPipelineColorBlendAttachmentState state) {
        uint32_t i = 0, idx = builder.get_attachment_index(name, false);
        for (const auto &reference : output_attachments) {
            if (reference.attachment == idx) {
                output_attachment_blend_states[i] = state;
                break;
            }
            i ++;
        }
    }

    void SubpassBuilder::wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other) {
        uint32_t src_subpass;
        if (name == EXTERNAL_SUBPASS) {
            src_subpass = VK_SUBPASS_EXTERNAL;
        } else {
            src_subpass = builder.get_subpass_index(name);
        }
        builder.final_dependencies.push_back({
            .srcSubpass = src_subpass,
            .dstSubpass = builder.get_subpass_index(this->name),
            .srcStageMask = other.stage,
            .dstStageMask = self.stage,
            .srcAccessMask = other.access,
            .dstAccessMask = self.access,
        });
    }

    VkSubpassDescription SubpassBuilder::build() const {
        VkSubpassDescription desc {};
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
            attachment.description_read->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        } else {
            attachment.description_write.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        }
    }

    void RenderPassBuilder::add_attachment(const std::string &name, AttachmentType type) {
        attachments_map.insert(std::make_pair(name, attachments.size()));
        auto &attachment = attachments.emplace_back();
        attachment.description_write.flags = 0;
        attachment.description_write.samples = runtime_setting->multisample_count;
        attachment.description_write.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.description_write.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.description_write.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.description_write.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.description_write.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.usage = VK_IMAGE_LAYOUT_UNDEFINED;
        switch (type) {
        case AttachmentType::eColor:
            attachment.description_write.format = manager->swapchain->format.format;
            attachment.description_write.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            attachment.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.clear_value = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } };
            if (runtime_setting->is_msaa_enabled()) {
                attachment.description_read = attachment.description_write;
                attachment.description_read->samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.description_read->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.usage_read = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                attachment.offset = resolved_attachment_count;
                resolved_attachment_count += 1;
            }
            break;
        case AttachmentType::eDepthBuffer:
            attachment.usage = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        case AttachmentType::eDepth:
            attachment.description_write.format = get_supported_depth_format();
            attachment.description_write.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachment.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            attachment.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (has_stencil_component(attachment.description_write.format)) {
                attachment.aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            attachment.clear_value = { .depthStencil = { 1.0f, 0 } };
            break;
        case AttachmentType::eFloat4Buffer:
            attachment.description_write.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attachment.description_write.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            attachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            attachment.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.clear_value = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } };
            if (runtime_setting->is_msaa_enabled()) {
                attachment.description_read = attachment.description_write;
                attachment.description_read->samples = VK_SAMPLE_COUNT_1_BIT;
                attachment.description_read->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment.description_write.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                attachment.usage_read = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
                attachment.offset = resolved_attachment_count;
                resolved_attachment_count += 1;
            } else {
                attachment.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            }
            break;
        }
    }

    SubpassBuilder &RenderPassBuilder::add_subpass(const std::string &name) {
        subpass_builders_map.insert(std::make_pair(name, subpass_builders.size()));
        subpass_builders.push_back(std::make_unique<SubpassBuilder>(name, *this));
        return *subpass_builders.back();
    }

    VkRenderPassCreateInfo RenderPassBuilder::build() {
        VkRenderPassCreateInfo create_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
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
        create_info.attachmentCount = final_attachments.size();
        create_info.pAttachments = final_attachments.data();
        final_subpasses.clear();
        final_subpasses.reserve(subpass_builders.size());
        for (const auto &subpass_builder : subpass_builders) {
            final_subpasses.push_back(subpass_builder->build());
        }
        create_info.subpassCount = final_subpasses.size();
        create_info.pSubpasses = final_subpasses.data();
        create_info.dependencyCount = final_dependencies.size();
        create_info.pDependencies = final_dependencies.data();
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
