#include <Match/vulkan/resource/shader.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    Shader::Shader(const std::vector<uint32_t> &code) {
        create(code.data(), code.size() * 4);
    }

    Shader::Shader(const std::vector<char> &code) {
        create(reinterpret_cast<const uint32_t *>(code.data()), code.size());
    }

    void Shader::create(const uint32_t *data, uint32_t size) {
        VkShaderModuleCreateInfo shader_module_create_info { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shader_module_create_info.pCode = data;
        shader_module_create_info.codeSize = size;
        vk_check(vkCreateShaderModule(manager->device->device, &shader_module_create_info, manager->allocator, &module))
    }

    Shader::~Shader() {
        vkDestroyShaderModule(manager->device->device, module, manager->allocator);
        layout_bindings.clear();
    }

    void Shader::bind_descriptors(const std::vector<DescriptorInfo> &descriptor_infos) {
        for (const auto &descriptor_info : descriptor_infos) {
            layout_bindings.push_back({
                .binding = descriptor_info.binding,
                .descriptorType = transform<VkDescriptorType>(descriptor_info.type),
                .descriptorCount = descriptor_info.count,
                .stageFlags = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM,  // set by shader program
            });
            if (descriptor_info.type == DescriptorType::eTexture) {
                layout_bindings.back().pImmutableSamplers = descriptor_info.spec_data.immutable_samplers;
            }
        }
    }
}
