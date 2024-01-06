#include <Match/vulkan/resource/shader.hpp>
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
        case Match::ShaderStage::eRayGen:
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
        constants_size = 0;
        vk::ShaderModuleCreateInfo shader_module_create_info {};
        shader_module_create_info.setPCode(data)
            .setCodeSize(size);
        module = manager->device->device.createShaderModule(shader_module_create_info);
    }

    bool Shader::is_ready() {
        return module.has_value();
    }

    Shader::~Shader() {
        constant_offset_map.clear();
        if (module.has_value()) {
            manager->device->device.destroyShaderModule(module.value());
            module.reset();

        }
    }

    Shader &Shader::bind_push_constants(const std::vector<ConstantInfo> &constant_infos) {
        for (auto info : constant_infos) {
            auto size = transform<uint32_t>(info.type);
            uint32_t align = 4;
            if (size > 8) {
                align = 16;
            } else if (size > 4) {
                align = 8;
            }
            if (!first_align.has_value()) {
                first_align = align;
            }
            if (constants_size % align != 0) {
                constants_size += align - (constants_size % align);
            }
            constant_offset_map.insert(std::make_pair(info.name, constants_size));
            constant_size_map.insert(std::make_pair(info.name, size));
            constants_size += size;
        }
        return *this;
    }
}
