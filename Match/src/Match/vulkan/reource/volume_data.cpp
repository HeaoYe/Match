#include <Match/vulkan/resource/volume_data.hpp>

namespace Match {
    VolumeData::VolumeData(const std::string &filename) {
        raw_data.resize(volume_raw_data_resolution * volume_raw_data_resolution * volume_raw_data_resolution);
        auto f = fopen(filename.c_str(), "rb");
        if (f == NULL) {
            return;
        }
        fread(raw_data.data(), sizeof(float), raw_data.size(), f);
        fclose(f);
    }

    VolumeData::VolumeData(const std::vector<float> &raw_data) : raw_data(raw_data) {
        while (this->raw_data.size() < volume_raw_data_resolution * volume_raw_data_resolution * volume_raw_data_resolution) {
            this->raw_data.push_back(0);
        }
    }

    float VolumeData::get(int x, int y, int z) const {
        return raw_data[xyz_to_index(x, y, z)];
    }

    void VolumeData::set(int x, int y, int z, float value) {
        raw_data[xyz_to_index(x, y, z)] = value;
    }

    void VolumeData::upload_to_buffer(std::shared_ptr<TwoStageBuffer> buffer, bool is_flush) {
        buffer->upload_data_from_vector(raw_data);
        if (is_flush) {
            buffer->flush();
        }
    }
}
