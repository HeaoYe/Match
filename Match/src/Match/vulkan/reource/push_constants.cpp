#include <Match/vulkan/resource/push_constants.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    PushConstants::PushConstants(ShaderStages stages, const std::vector<PushConstantInfo> &infos) {
        constants_size = 0;
        for (auto &info : infos) {
            auto size = transform<uint32_t>(info.type);
            uint32_t align = 4;
            if (size > 8) {
                align = 16;
            } else if (size > 4) {
                align = 8;
            }
            if (constants_size % align != 0) {
                constants_size += align - (constants_size % align);
            }
            constant_offset_map.insert(std::make_pair(info.name, constants_size));
            constant_size_map.insert(std::make_pair(info.name, size));
            constants_size += size;
        }
        range.setStageFlags(transform<vk::ShaderStageFlags>(stages))
            .setOffset(0)
            .setSize(constants_size);
        constants.resize(constants_size);
    }

    void PushConstants::push_constant(const std::string &name, BasicConstantValue basic_value) {
        push_constant(name, &basic_value);
    }

    void PushConstants::push_constant(const std::string &name, void *data) {
        memcpy(constants.data() + constant_offset_map.at(name), data, constant_size_map.at(name));
    }

    PushConstants::~PushConstants() {
        constant_offset_map.clear();
        constant_size_map.clear();
        constants.clear();
    }
}
