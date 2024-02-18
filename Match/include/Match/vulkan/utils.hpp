#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    MATCH_API vk::ImageView create_image_view(vk::Image image, vk::Format format, vk::ImageAspectFlags aspect, uint32_t mipmap_levels, uint32_t layer_count = 1, vk::ImageViewType view_type = vk::ImageViewType::e2D);

    MATCH_API vk::Format find_supported_format(const std::vector<vk::Format> &candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    struct MATCH_API TransitionInfo {
        vk::ImageLayout layout;
        vk::AccessFlags access;
        vk::PipelineStageFlags stage;
    };

    MATCH_API void transition_image_layout(vk::Image image, vk::ImageAspectFlags aspect, uint32_t mip_levels, const TransitionInfo &src, const TransitionInfo &dst);

    MATCH_API vk::Format get_supported_depth_format();

    MATCH_API bool has_stencil_component(vk::Format format);

    MATCH_API SampleCount get_max_usable_sample_count();

    MATCH_API vk::DeviceAddress get_buffer_address(vk::Buffer buffer);

    MATCH_API vk::DeviceAddress get_acceleration_structure_address(vk::AccelerationStructureKHR acceleration_structure);
}
