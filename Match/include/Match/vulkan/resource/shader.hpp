#pragma once

#include <Match/vulkan/commons.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <optional>

namespace Match {
    enum class DescriptorType {
        eUniform,
        eTexture,
        eTextureAttachment,
        eInputAttachment,
    };

    struct DescriptorInfo {
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count, const VkSampler *immutable_samplers) : binding(binding), type(type), count(count), immutable_samplers(immutable_samplers) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, uint32_t count) : binding(binding), type(type), count(count) {}
        DescriptorInfo(uint32_t binding, DescriptorType type, const VkSampler *immutable_samplers) : binding(binding), type(type), count(1), immutable_samplers(immutable_samplers) {}
        DescriptorInfo(uint32_t binding, DescriptorType type) : binding(binding), type(type), count(1), immutable_samplers(nullptr) {}
        uint32_t binding;
        DescriptorType type;
        uint32_t count;
        const VkSampler *immutable_samplers;
    };

    typedef VertexType ConstantType;

    struct ConstantInfo {
        std::string name;
        ConstantType type;
    };

    enum class ShaderType {
        eCompiled,
        eVertexShaderNeedCompile,
        eFragmentShaderNeedCompile,
    };

    class Shader {
        no_copy_move_construction(Shader)
        using binding = uint32_t;
    public:
        Shader(const std::string &name, const std::string &code, ShaderType type);
        Shader(const std::vector<char> &code);
        void bind_descriptors(const std::vector<DescriptorInfo> &descriptor_infos);
        void bind_push_constants(const std::vector<ConstantInfo> &constant_infos);
        bool is_ready();
        ~Shader();
    private:
        void create(const uint32_t *data, uint32_t size);
    INNER_VISIBLE:
        VkDescriptorSetLayoutBinding *get_layout_binding(uint32_t binding);
    INNER_VISIBLE:
        VkShaderModule module;
        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        std::optional<uint32_t> first_align;
        uint32_t constants_size;
        std::map<std::string, uint32_t> constant_size_map;
        std::map<std::string, uint32_t> constant_offset_map;
    };
}
