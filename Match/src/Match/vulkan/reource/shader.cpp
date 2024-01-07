#include <Match/vulkan/resource/shader.hpp>
#include <Match/vulkan/resource/shader_includer.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"
#include <shaderc/shaderc.hpp>

namespace Match {
    Shader::Shader(const std::string &name, const std::string &code, ShaderStage stage) {
        shaderc_shader_kind kind;
        switch (stage) {
        case Match::ShaderStage::eVertex:
            kind = shaderc_glsl_vertex_shader;
            break;
        case Match::ShaderStage::eFragment:
            kind = shaderc_glsl_fragment_shader;
            break;
        case Match::ShaderStage::eRaygen:
            kind = shaderc_glsl_raygen_shader;
            break;
        case Match::ShaderStage::eMiss:
            kind = shaderc_glsl_miss_shader;
            break;
        case Match::ShaderStage::eClosestHit:
            kind = shaderc_glsl_closesthit_shader;
            break;
        default:
            throw std::runtime_error("");
        }
        shaderc::Compiler compiler {};
        shaderc::CompileOptions options {};
        options.SetIncluder(std::make_unique<ShaderIncluder>());
        if (setting.enable_ray_tracing) {
           options.SetTargetSpirv(shaderc_spirv_version_1_6);
        }
        auto module = compiler.CompileGlslToSpv(code.data(), code.size(), kind, name.c_str(), options);
        if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
            MCH_ERROR("Compile {} faild: {}", name, module.GetErrorMessage());
            return;
        }
        std::vector<uint32_t> spirv(module.cbegin(), module.cend());
        create(spirv.data(), spirv.size() * 4);
    }

    Shader::Shader(const std::vector<char> &code) {
        create(reinterpret_cast<const uint32_t *>(code.data()), code.size() / 4);
    }

    void Shader::create(const uint32_t *data, uint32_t size) {
        vk::ShaderModuleCreateInfo shader_module_create_info {};
        shader_module_create_info.setPCode(data)
            .setCodeSize(size);
        module = manager->device->device.createShaderModule(shader_module_create_info);
    }

    bool Shader::is_ready() {
        return module.has_value();
    }

    Shader::~Shader() {
        if (module.has_value()) {
            manager->device->device.destroyShaderModule(module.value());
            module.reset();

        }
    }
}
