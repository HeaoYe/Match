#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    vk::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, uint32_t mipmap_levels, uint32_t layer_count = 1, vk::ImageViewType view_type = vk::ImageViewType::e2D);

    vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    struct TransitionInfo {
        vk::ImageLayout layout;
        vk::AccessFlags access;
        vk::PipelineStageFlags stage;
    };

    void transition_image_layout(vk::Image image, vk::ImageAspectFlags aspect, uint32_t mip_levels, const TransitionInfo &src, const TransitionInfo &dst);

    vk::Format get_supported_depth_format();

    bool has_stencil_component(vk::Format format);

    SampleCount get_max_usable_sample_count();
}
