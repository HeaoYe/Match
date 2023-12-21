#pragma once

#include <Match/vulkan/commons.hpp>

namespace Match {
    enum class DescriptorType {
        eUniform,
        eTexture,
    };

    struct DescriptorInfo {
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count, const VkSampler *immutable_samplers) : binding(binding), type(type), count(count), spec_data({.immutable_samplers = immutable_samplers}) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count, uint32_t size) : binding(binding), type(type), count(count), spec_data({.size = size}) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, const VkSampler *immutable_samplers) : binding(binding), type(type), count(1), spec_data({.immutable_samplers = immutable_samplers}) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t size) : binding(binding), type(type), count(1), spec_data({.size = size}) {}
        DescriptorInfo(uint32_t binding, DescriptorType type) : binding(binding), type(type), count(1), spec_data({.immutable_samplers = nullptr}) {}
        uint32_t binding;
        DescriptorType type;
        uint32_t count;
        union {
            uint32_t size;
            const VkSampler *immutable_samplers;
        } spec_data;
    };

    class Shader {
        no_copy_move_construction(Shader)
        using binding = uint32_t;
    public:
        Shader(const std::vector<char> &code);
        Shader(const std::vector<uint32_t> &code);
        void bind_descriptors(const std::vector<DescriptorInfo> &descriptor_infos);
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        VkShaderModule module;
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    };
}
