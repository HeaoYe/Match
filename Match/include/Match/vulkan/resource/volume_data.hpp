#pragma once

#include <Match/vulkan/resource/buffer.hpp>

namespace Match {
    constexpr size_t volume_raw_data_resolution = 1000;

    class MATCH_API VolumeData {
        no_copy_move_construction(VolumeData)
    public:
        VolumeData(const std::string &filename);
        VolumeData(const std::vector<float> &raw_data);

        float get(int x, int y, int z) const;
        void set(int x, int y, int z, float value);

        void upload_to_buffer(std::shared_ptr<TwoStageBuffer> buffer, bool is_flush = true);
    private:
        int xyz_to_index(int x, int y, int z) const {
            return x * volume_raw_data_resolution * volume_raw_data_resolution + y * volume_raw_data_resolution + z;
        }
    INNER_VISIBLE:
        std::vector<float> raw_data;
    };
}
