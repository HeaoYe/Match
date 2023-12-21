#pragma once

#include <Match/vulkan/resource/image.hpp>

namespace Match {
    class Texture {
        no_copy_move_construction(Texture)
    public:
        Texture(const std::string &filename);
        ~Texture();
    INNER_VISIBLE:
        std::unique_ptr<Image> image;
        VkImageView image_view;
    };
}