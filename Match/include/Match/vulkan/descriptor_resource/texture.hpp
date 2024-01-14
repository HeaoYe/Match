#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Texture {
        default_no_copy_move_construction(Texture)
    public:
        virtual ~Texture() = default;
        virtual vk::ImageLayout get_image_layout() = 0;
        virtual vk::ImageView get_image_view() = 0;
        virtual uint32_t get_mip_levels() = 0;
    protected:
        void load_error(const std::string &filename);
    };
}
