#pragma once

#include <Match/vulkan/commons.hpp>
#include <optional>

namespace Match {
    struct AttachmentInfo {
        vk::AttachmentDescription description_write;
        vk::ImageUsageFlags usage;
        std::optional<vk::AttachmentDescription> description_read;
        vk::ImageUsageFlags usage_read;
        uint32_t offset;
        vk::ImageAspectFlags aspect;
        vk::ClearValue clear_value;
    };

    class RenderPassBuilder;

    class SubpassBuilder {
        no_copy_move_construction(SubpassBuilder)
    public:
        SubpassBuilder(const std::string &name, RenderPassBuilder &builder) : name(name), builder(builder) {}
        SubpassBuilder &bind(vk::PipelineBindPoint bind_point);
        SubpassBuilder &attach_input_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal);
        SubpassBuilder &attach_output_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);
        SubpassBuilder &attach_depth_attachment(const std::string &name, vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);
        SubpassBuilder &attach_preserve_attachment(const std::string &name);
        SubpassBuilder &set_output_attachment_color_blend(const std::string &name, vk::PipelineColorBlendAttachmentState state);
        SubpassBuilder &wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other);
        vk::SubpassDescription build() const;
    private:
        vk::AttachmentReference create_reference(const std::string &name, vk::ImageLayout layout, bool is_attachment_read);
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
        RenderPassBuilder();
        RenderPassBuilder &add_attachment(const std::string &name, AttachmentType type);
        SubpassBuilder &add_subpass(const std::string &name);
        vk::RenderPassCreateInfo build();
    INNER_VISIBLE:
        uint32_t get_attachment_index(const std::string &name, bool is_attachment_read);
        uint32_t get_subpass_index(const std::string &name);
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
