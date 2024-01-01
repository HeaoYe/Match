#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <optional>

namespace Match {
    struct DescriptorInfo {
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count, const vk::Sampler *immutable_samplers) : binding(binding), type(type), count(count), immutable_samplers(immutable_samplers) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count) : binding(binding), type(type), count(count) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, const vk::Sampler *immutable_samplers) : binding(binding), type(type), count(1), immutable_samplers(immutable_samplers) {}
        DescriptorInfo(uint32_t binding, DescriptorType type) : binding(binding), type(type), count(1), immutable_samplers(nullptr) {}
        uint32_t binding;
        DescriptorType type;
        uint32_t count;
        const vk::Sampler *immutable_samplers;
    };

    struct ConstantInfo {
        std::string name;
        ConstantType type;
    };

    class Shader {
        no_copy_move_construction(Shader)
        using binding = uint32_t;
    public:
        Shader(const std::string &name, const std::string &code, ShaderType type);
        Shader(const std::vector<char> &code);
        Shader &bind_descriptors(const std::vector<DescriptorInfo> &descriptor_infos);
        Shader &bind_push_constants(const std::vector<ConstantInfo> &constant_infos);
        bool is_ready();
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        vk::DescriptorSetLayoutBinding *get_layout_binding(uint32_t binding);
    INNER_VISIBLE:
        vk::ShaderModule module;
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        std::optional<uint32_t> first_align;
        uint32_t constants_size;
        std::map<std::string, uint32_t> constant_size_map;
        std::map<std::string, uint32_t> constant_offset_map;
    };
}
