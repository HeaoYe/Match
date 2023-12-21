#include <Match/vulkan/renderpass.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/constant.hpp>
#include "inner.hpp"

namespace Match {
    SubpassBuilder::SubpassBuilder(SubpassBuilder &&rhs) : builder(rhs.builder) {
        name = std::move(rhs.name);
        bind_point = rhs.bind_point;
        input_attachments = std::move(rhs.input_attachments);
        output_attachments = std::move(rhs.output_attachments);
        resolve_attachments = std::move(rhs.resolve_attachments);
        preserve_attachments = std::move(rhs.preserve_attachments);
        depth_attachment = std::move(rhs.depth_attachment);
    }

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
        output_attachments.push_back(create_reference(name, layout));
        output_attachment_blend_states.push_back({
            .blendEnable = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });
    }
    
    void SubpassBuilder::attach_resolve_attachment(const std::string &name, VkImageLayout layout) {
        resolve_attachments.push_back(create_reference(name, layout));
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

    void SubpassBuilder::wait_for(const std::string &name, AccessInfo self, AccessInfo other) {
        uint32_t src_subpass;
        if (name == EXTERNAL_SUBPASS) {
            src_subpass = VK_SUBPASS_EXTERNAL;
        } else {
            src_subpass = builder.subpasses_map.at(name);
        }
        builder.dependencies.push_back({
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
        desc.pResolveAttachments = resolve_attachments.data();
        if (depth_attachment.has_value()) {
            desc.pDepthStencilAttachment = &depth_attachment.value();
        }
        desc.preserveAttachmentCount = preserve_attachments.size();
        desc.pPreserveAttachments = preserve_attachments.data();
        return std::move(desc);
    }

    void RenderPassBuilder::add_attachment(const std::string &name, AttchmentType type) {
        VkAttachmentDescription attachment {
            .flags = 0,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };
        switch (type) {
        case AttchmentType::eColor:
            attachment.samples = runtime_setting->get_multisample_count();
            attachment.format = manager->swapchain->format.format;
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            break;
        case AttchmentType::eDepth:
            attachment.samples = runtime_setting->get_multisample_count();
            attachment.format = get_supported_depth_format();
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            break;
        }
        add_custom_attachment(name, attachment);
    }

    void RenderPassBuilder::add_custom_attachment(const std::string &name, VkAttachmentDescription desc) {
        attachments_map.insert(std::make_pair(name, attachments.size()));
        attachments.push_back(std::move(desc));
    }

    void RenderPassBuilder::set_final_present_attachment(const std::string &name) {
        attachments[attachments_map.at(name)].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }

    SubpassBuilder &RenderPassBuilder::create_subpass(const std::string &name) {
        subpasses_map.insert(std::make_pair(name, subpass_builders.size()));
        return subpass_builders.emplace_back(name, *this);
    }

    VkRenderPassCreateInfo RenderPassBuilder::build() {
        VkRenderPassCreateInfo create_info { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        create_info.attachmentCount = attachments.size();
        create_info.pAttachments = attachments.data();
        subpasses.clear();
        for (const auto &subpass_builder : subpass_builders) {
            subpasses.push_back(std::move(subpass_builder.build()));
        }
        create_info.subpassCount = subpasses.size();
        create_info.pSubpasses = subpasses.data();
        create_info.dependencyCount = dependencies.size();
        create_info.pDependencies = dependencies.data();
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
