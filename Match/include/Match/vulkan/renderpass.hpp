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
        VkPipelineStageFlagBits stage;
        VkAccessFlagBits access;
    };

    class SubpassBuilder {
        no_copy_construction(SubpassBuilder)
    public:
        SubpassBuilder(const std::string &name, RenderPassBuilder &builder) : name(name), builder(builder) {}
        SubpassBuilder(SubpassBuilder &&rhs);
        void bind(VkPipelineBindPoint bind_point);
        void attach_input_attachment(const std::string &name, VkImageLayout layout);

        void attach_output_attachment(const std::string &name, VkImageLayout layout);
        void attach_resolve_attachment(const std::string &name, VkImageLayout layout);
        
        void attach_preserve_attachment(const std::string &name);
        void attach_depth_attachment(const std::string &name, VkImageLayout layout);

        void set_output_attachment_color_blend(const std::string &name, VkPipelineColorBlendAttachmentState state);
        
        void wait_for(const std::string &name, AccessInfo self, AccessInfo other);
        
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

    class APIManager;
    class RenderPassBuilder {
        default_no_copy_move_construction(RenderPassBuilder)
    public:
        void add_attachment(const std::string &name, AttchmentType type);
        void add_custom_attachment(const std::string &name, VkAttachmentDescription desc);
        void set_final_present_attachment(const std::string &name);
        SubpassBuilder &create_subpass(const std::string &name);
        VkRenderPassCreateInfo build();
    INNER_VISIBLE:
        std::vector<VkSubpassDescription> subpasses;
        std::vector<VkAttachmentDescription> attachments;
        std::map<std::string, uint32_t> attachments_map;
        std::vector<SubpassBuilder> subpass_builders;
        std::map<std::string, uint32_t> subpasses_map;
        std::vector<VkSubpassDependency> dependencies;
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
