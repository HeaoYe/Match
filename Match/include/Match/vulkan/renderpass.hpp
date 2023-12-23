#pragma once

#include <Match/vulkan/commons.hpp>
#include <optional>

namespace Match {
    enum class AttchmentType {
        eColor,
        eDepth,
    };

    class RenderPassBuilder;

    struct AccessInfo {
        VkPipelineStageFlags stage;
        VkAccessFlags access;
    };

    class SubpassBuilder {
        no_copy_construction(SubpassBuilder)
    public:
        SubpassBuilder(const std::string &name, RenderPassBuilder &builder) : name(name), builder(builder) {}
        SubpassBuilder(SubpassBuilder &&rhs);
        void bind(VkPipelineBindPoint bind_point);
        void attach_input_attachment(const std::string &name, VkImageLayout layout);

        void attach_output_attachment(const std::string &name, VkImageLayout layout);
        // void attach_resolve_attachment(const std::string &name, VkImageLayout layout);
        
        void attach_preserve_attachment(const std::string &name);
        void attach_depth_attachment(const std::string &name, VkImageLayout layout);

        void set_output_attachment_color_blend(const std::string &name, VkPipelineColorBlendAttachmentState state);
        
        void wait_for(const std::string &name, const AccessInfo &self, const AccessInfo &other);
        
        VkSubpassDescription build() const;
    private:
        VkAttachmentReference create_reference(const std::string &name, VkImageLayout layout);
    INNER_VISIBLE:
        std::string name;
        RenderPassBuilder &builder;
        VkPipelineBindPoint bind_point;
        std::vector<VkAttachmentReference> input_attachments;
        std::vector<VkAttachmentReference> output_attachments;
        std::vector<VkPipelineColorBlendAttachmentState> output_attachment_blend_states;
        std::vector<VkAttachmentReference> resolve_attachments;
        std::vector<uint32_t> preserve_attachments;
        std::optional<VkAttachmentReference> depth_attachment;
    };

    struct AttachmentDescription {
        VkAttachmentDescription description;
        std::optional<VkAttachmentDescription> resolve_description;
        uint32_t offset;
        VkImageUsageFlags usage;
        VkImageAspectFlags aspect;
        VkClearValue clear_value;
    };

    class APIManager;
    class RenderPassBuilder {
        default_no_copy_move_construction(RenderPassBuilder)
    public:
        void add_attachment(const std::string &name, AttchmentType type);
        SubpassBuilder &add_subpass(const std::string &name);
        VkRenderPassCreateInfo build();
    INNER_VISIBLE:
        std::vector<AttachmentDescription> attachments;
        std::map<std::string, uint32_t> attachments_map;
        uint32_t color_attachment_count = 0;
        // std::map<uint32_t, std::pair<VkAttachmentDescription, uint32_t>> resolve_attachments;
        // std::vector<std::pair<VkImageUsageFlags, VkImageAspectFlags>> attachment_infos;
        // std::vector<VkClearValue> clear_values;
        std::vector<SubpassBuilder> subpass_builders;
        std::map<std::string, uint32_t> subpasses_map;

        std::vector<VkSubpassDescription> final_subpasses;
        std::vector<VkAttachmentDescription> final_attachments;
        std::vector<VkSubpassDependency> final_dependencies;
    };

    class RenderPass {
        no_copy_move_construction(RenderPass)
    public:
        RenderPass(std::shared_ptr<RenderPassBuilder> builder);
        ~RenderPass();
    INNER_VISIBLE:
        VkRenderPass render_pass;
    };
}
