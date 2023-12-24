#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    class Texture {
        default_no_copy_move_construction(Texture)
    public:
        virtual ~Texture() = default;
        virtual VkImage get_image() = 0;
        virtual VkImageView get_image_view() = 0;
        virtual uint32_t get_mip_levels() = 0;
    protected:
        void load_error(const std::string &filename);
    };
}
