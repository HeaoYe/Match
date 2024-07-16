#pragma once

#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    constexpr size_t volume_raw_data_resolution = 640;

    class VolumeData {
        no_copy_move_construction(VolumeData)
    public:
        MATCH_API VolumeData(const std::string &filename);
        MATCH_API VolumeData(const std::vector<float> &raw_data);

        MATCH_API float get(int x, int y, int z) const;
        MATCH_API void set(int x, int y, int z, float value);

        MATCH_API void upload_to_buffer(std::shared_ptr<TwoStageBuffer> buffer, bool is_flush = true);
    private:
        int xyz_to_index(int x, int y, int z) const {
            return x * volume_raw_data_resolution * volume_raw_data_resolution + y * volume_raw_data_resolution + z;
        }
    INNER_VISIBLE:
        std::vector<float> raw_data;
    };
}
