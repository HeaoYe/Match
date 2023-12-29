#pragma once

#include <Match/vulkan/commons.hpp>
#include <optional>

namespace Match {
    enum class AttachmentType {
        eColor,
        eDepth,
        eDepthBuffer,
        eColorBuffer,
    };

    struct AttachmentInfo {
        VkAttachmentDescription description_write;
        VkImageUsageFlags usage;
        std::optional<VkAttachmentDescription> description_read;
        VkImageUsageFlags usage_read;
        uint32_t offset;
        VkImageAspectFlags aspect;
        VkClearValue clear_value;
    };

    struct AccessInfo {
        VkPipelineStageFlags stage;
        VkAccessFlags access;
    };

    class RenderPassBuilder;

    class SubpassBuilder {
        no_copy_move_construction(SubpassBuilder)
    public:
        SubpassBuilder(const std::string &name, RenderPassBuilder &builder) : name(name), builder(builder) {}
        void bind(VkPipelineBindPoint bind_point);
        void attach_input_attachment(const std::string &name, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void attach_output_attachment(const std::string &name, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        void attach_depth_attachment(const std::string &name, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
        void attach_preserve_attachment(const std::string &name);
        void set_output_attachment_color_blend(const std::string &name, VkPipelineColorBlendAttachmentState state);
        void wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other);
        VkSubpassDescription build() const;
    private:
        VkAttachmentReference create_reference(const std::string &name, VkImageLayout layout, bool is_attachment_read);
    INNER_VISIBLE:
        std::string name;
        RenderPassBuilder &builder;
        VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
        std::vector<VkAttachmentReference> input_attachments;
        std::vector<VkAttachmentReference> output_attachments;
        std::vector<VkPipelineColorBlendAttachmentState> output_attachment_blend_states;
        std::vector<VkAttachmentReference> resolve_attachments;
        std::vector<uint32_t> preserve_attachments;
        std::optional<VkAttachmentReference> depth_attachment;
    };

    class RenderPassBuilder {
        no_copy_move_construction(RenderPassBuilder)
    public:
        RenderPassBuilder();
        void add_attachment(const std::string &name, AttachmentType type);
        SubpassBuilder &add_subpass(const std::string &name);
        VkRenderPassCreateInfo build();
    INNER_VISIBLE:
        uint32_t get_attachment_index(const std::string &name, bool is_attachment_read);
        uint32_t get_subpass_index(const std::string &name);
    INNER_VISIBLE:
        std::vector<AttachmentInfo> attachments;
        std::map<std::string, uint32_t> attachments_map;
        uint32_t resolved_attachment_count = 0;

        std::vector<std::unique_ptr<SubpassBuilder>> subpass_builders;
        std::map<std::string, uint32_t> subpass_builders_map;

        std::vector<VkAttachmentDescription> final_attachments;
        std::vector<VkSubpassDescription> final_subpasses;
        std::vector<VkSubpassDependency> final_dependencies;
    };
};
