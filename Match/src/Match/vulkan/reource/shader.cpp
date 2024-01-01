#include <Match/vulkan/resource/shader.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"
#include <shaderc/shaderc.hpp>

namespace Match {
    Shader::Shader(const std::string &name, const std::string &code, ShaderType type) {
        if (type != ShaderType::eCompiled) {
            shaderc_shader_kind kind;
            switch (type) {
            case Match::ShaderType::eVertexShaderNeedCompile:
                kind = shaderc_glsl_vertex_shader;
                break;
            case Match::ShaderType::eFragmentShaderNeedCompile:
                kind = shaderc_glsl_fragment_shader;
                break;
            default:
                throw std::runtime_error("");
            }
            shaderc::Compiler compiler;
            auto module = compiler.CompileGlslToSpv(code.data(), code.size(), kind, name.c_str(), {});
            if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
                MCH_ERROR("Compile {} faild: {}", name, module.GetErrorMessage());
            }
            std::vector<uint32_t> spirv(module.cbegin(), module.cend());
            create(spirv.data(), spirv.size() * 4);
        } else {
            MCH_FATAL("Compile Shader Error: Unknown Shader Type {}", static_cast<uint32_t>(type));
        }
    }

    Shader::Shader(const std::vector<char> &code) {
        create(reinterpret_cast<const uint32_t *>(code.data()), code.size());
    }

    void Shader::create(const uint32_t *data, uint32_t size) {
        constants_size = 0;
        vk::ShaderModuleCreateInfo shader_module_create_info {};
        shader_module_create_info.setPCode(data)
            .setCodeSize(size);
        module = VK_NULL_HANDLE;
        module = manager->device->device.createShaderModule(shader_module_create_info);
    }

    vk::DescriptorSetLayoutBinding *Shader::get_layout_binding(uint32_t binding) {
        for (auto &layout_binding : layout_bindings) {
            if (layout_binding.binding == binding) {
                return &layout_binding;
            }
        }
        return nullptr;
    }
    
    bool Shader::is_ready() {
        return module != VK_NULL_HANDLE;
    }

    Shader::~Shader() {
        constant_offset_map.clear();
        layout_bindings.clear();
        manager->device->device.destroyShaderModule(module);
    }

    Shader &Shader::bind_descriptors(const std::vector<DescriptorInfo> &descriptor_infos) {
        for (const auto &descriptor_info : descriptor_infos) {
            layout_bindings.push_back({
                descriptor_info.binding,
                transform<vk::DescriptorType>(descriptor_info.type),
                descriptor_info.count,
                vk::ShaderStageFlagBits::eAllGraphics,  // set by shader program
            });
            if (descriptor_info.type == DescriptorType::eTexture) {
                layout_bindings.back().pImmutableSamplers = descriptor_info.immutable_samplers;
            }
        }
        return *this;
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
