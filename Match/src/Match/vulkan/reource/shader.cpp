#include <Match/vulkan/resource/shader.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    Shader::Shader(const std::string &filename) {
        auto code = read_binary_file(filename);
        VkShaderModuleCreateInfo shader_module_create_info { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
        shader_module_create_info.pCode = reinterpret_cast<const uint32_t *>(code.data());
        shader_module_create_info.codeSize = code.size();
        vk_check(vkCreateShaderModule(manager->device->device, &shader_module_create_info, manager->alloctor, &module))
    }

    Shader::~Shader() {
        vkDestroyShaderModule(manager->device->device, module, manager->alloctor);
    }
}
