#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    VkAttachmentReference SubpassBuilder::create_reference(const std::string &name, VkImageLayout layout) {
        return { builder.attachments_map.at(name), layout };
    }
    
    void SubpassBuilder::bind(VkPipelineBindPoint bind_point) {
        this->bind_point = bind_point;
    }

    void SubpassBuilder::attach_input_attachment(const std::string &name, VkImageLayout layout) {
        input_attachments.push_back(create_reference(name, layout));
    }

    void SubpassBuilder::attach_output_attachment(const std::string &name, VkImageLayout layout) {
        auto reference = create_reference(name, layout);
        output_attachments.push_back(reference);
        output_attachment_blend_states.push_back({
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });
        reference.attachment = builder.attachments.size() + builder.attachments[reference.attachment].offset;
        resolve_attachments.push_back(reference);
    }
    
    void SubpassBuilder::attach_preserve_attachment(const std::string &name) {
        preserve_attachments.push_back(builder.attachments_map.at(name));
    }
    
    void SubpassBuilder::attach_depth_attachment(const std::string &name, VkImageLayout layout) {
        depth_attachment = create_reference(name, layout);
    }

    void SubpassBuilder::set_output_attachment_color_blend(const std::string &name, VkPipelineColorBlendAttachmentState state) {
        uint32_t i = 0, idx = builder.attachments_map.at(name);
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
            src_subpass = builder.subpasses_map.at(name);
        }
        builder.final_dependencies.push_back({
            .srcSubpass = src_subpass,
            .dstSubpass = builder.subpasses_map.at(this->name),
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

    void RenderPassBuilder::add_attachment(const std::string &name, AttchmentType type) {
        attachments_map.insert(std::make_pair(name, attachments.size()));
        auto &attachment = attachments.emplace_back();
        attachment.description.flags = 0;
        attachment.description.samples = runtime_setting->multisample_count;
        attachment.description.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.description.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.description.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        switch (type) {
        case AttchmentType::eColor:
            attachment.description.format = manager->swapchain->format.format;
            attachment.description.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            attachment.aspect = VK_IMAGE_ASPECT_COLOR_BIT;
            attachment.clear_value = { .color = { .float32 = { 0.0f, 0.0f, 0.0f, 1.0f } } };

            attachment.resolve_description = attachment.description;
            attachment.resolve_description->samples = VK_SAMPLE_COUNT_1_BIT;
            attachment.resolve_description->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment.offset = color_attachment_count;
            color_attachment_count += 1;

            if (name == SWAPCHAIN_IMAGE_ATTACHMENT) {
                if (runtime_setting->is_msaa_enabled()) {
                    attachment.resolve_description->finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                } else {
                    attachment.description.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
                }
            }
            break;
        case AttchmentType::eDepth:
            attachment.description.format = get_supported_depth_format();
            attachment.description.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachment.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            attachment.aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (has_stencil_component(attachment.description.format)) {
                attachment.aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            attachment.clear_value = { .depthStencil = { 1.0f, 0 } };
            break;
        }
    }

    SubpassBuilder &RenderPassBuilder::add_subpass(const std::string &name) {
        subpasses_map.insert(std::make_pair(name, subpass_builders.size()));
        subpass_builders.push_back(std::make_unique<SubpassBuilder>(name, *this));
        return *subpass_builders.back();
    }

    VkRenderPassCreateInfo RenderPassBuilder::build() {
        VkRenderPassCreateInfo create_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        final_attachments.clear();
        final_attachments.reserve(attachments.size() + color_attachment_count);
        for (const auto &attachment : attachments) {
            final_attachments.push_back(attachment.description);
        }
        for (const auto &attachment : attachments) {
            if (runtime_setting->is_msaa_enabled() && attachment.resolve_description.has_value()) {
                final_attachments.push_back(attachment.resolve_description.value());
            }
        }
        create_info.attachmentCount = final_attachments.size();
        create_info.pAttachments = final_attachments.data();
        final_subpasses.clear();
        for (const auto &subpass_builder : subpass_builders) {
            final_subpasses.push_back(std::move(subpass_builder->build()));
        }
        create_info.subpassCount = final_subpasses.size();
        create_info.pSubpasses = final_subpasses.data();
        create_info.dependencyCount = final_dependencies.size();
        create_info.pDependencies = final_dependencies.data();
        return std::move(create_info);
    }

    RenderPass::RenderPass(std::shared_ptr<RenderPassBuilder> builder) {
        auto create_info = builder->build();
        vk_assert(vkCreateRenderPass(manager->device->device, &create_info, manager->allocator, &render_pass));
    }

    RenderPass::~RenderPass() {
        vkDestroyRenderPass(manager->device->device, render_pass, manager->allocator);
    }
}
