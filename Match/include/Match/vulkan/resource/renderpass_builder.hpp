#pragma once

#include <Match/vulkan/commons.hpp>
#include <optional>

namespace Match {
    struct AttachmentInfo {
        vk::AttachmentDescription description_write {};
        vk::ImageUsageFlags usage {};
        std::optional<vk::AttachmentDescription> description_read {};
        vk::ImageUsageFlags usage_read {};
        uint32_t offset {};
        vk::ImageAspectFlags aspect {};
        vk::ClearValue clear_value {};
    };

    class RenderPassBuilder;

    class SubpassBuilder {
        no_copy_move_construction(SubpassBuilder)
    public:
        SubpassBuilder(const std::string &name, RenderPassBuilder &builder) : name(name), builder(builder) {}
        MATCH_API SubpassBuilder &bind(vk::PipelineBindPoint bind_point);
        MATCH_API SubpassBuilder &attach_input_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal);
        MATCH_API SubpassBuilder &attach_output_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);
        MATCH_API SubpassBuilder &attach_depth_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);
        MATCH_API SubpassBuilder &attach_preserve_attachment(const std::string &name);
        MATCH_API SubpassBuilder &set_output_attachment_color_blend(const std::string &name, vk::PipelineColorBlendAttachmentState state);
        MATCH_API SubpassBuilder &wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other);
        MATCH_API vk::SubpassDescription build() const;
    private:
        MATCH_API vk::AttachmentReference create_reference(const std::string &name, vk::ImageLayout layout, bool is_attachment_read);
    INNER_VISIBLE:
        std::string name;
        RenderPassBuilder &builder;
        vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;
        std::vector<vk::AttachmentReference> input_attachments;
        std::vector<vk::AttachmentReference> output_attachments;
        std::vector<vk::PipelineColorBlendAttachmentState> output_attachment_blend_states;
        std::vector<vk::AttachmentReference> resolve_attachments;
        std::vector<uint32_t> preserve_attachments;
        std::optional<vk::AttachmentReference> depth_attachment;
    };

    class RenderPassBuilder {
        no_copy_move_construction(RenderPassBuilder)
    public:
        MATCH_API RenderPassBuilder();
        MATCH_API RenderPassBuilder &add_attachment(const std::string &name, AttachmentType type, vk::ImageUsageFlags additional_usage = {});
        MATCH_API SubpassBuilder &add_subpass(const std::string &name);
        MATCH_API vk::RenderPassCreateInfo build();
    INNER_VISIBLE:
        uint32_t get_attachment_index(const std::string &name, bool is_attachment_read) {
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

        uint32_t get_subpass_index(const std::string &name) {
            auto idx = subpass_builders_map.find(name);
            if (idx == subpass_builders_map.end()) {
                MCH_ERROR("No subpass named {}", name);
                return -1u;
            }
            return idx->second;
        }
    INNER_VISIBLE:
        std::vector<AttachmentInfo> attachments;
        std::map<std::string, uint32_t> attachments_map;
        uint32_t resolved_attachment_count = 0;

        std::vector<std::unique_ptr<SubpassBuilder>> subpass_builders;
        std::map<std::string, uint32_t> subpass_builders_map;

        std::vector<vk::AttachmentDescription> final_attachments;
        std::vector<vk::SubpassDescription> final_subpasses;
        std::vector<vk::SubpassDependency> final_dependencies;
    };
};
