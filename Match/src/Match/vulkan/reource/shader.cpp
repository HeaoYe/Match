#include <Match/vulkan/resource/shader.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>

namespace Match {
    Shader::Shader(const std::string &name, const std::vector<char> &code, ShaderStage stage) {
        EShLanguage kind;
        switch (stage) {
        case Match::ShaderStage::eVertex:
            kind = EShLanguage::EShLangVertex;
            break;
        case Match::ShaderStage::eFragment:
            kind = EShLanguage::EShLangFragment;
            break;
        case Match::ShaderStage::eRaygen:
            kind = EShLanguage::EShLangRayGen;
            break;
        case Match::ShaderStage::eMiss:
            kind = EShLanguage::EShLangMiss;
            break;
        case Match::ShaderStage::eClosestHit:
            kind = EShLanguage::EShLangClosestHit;
            break;
        case Match::ShaderStage::eIntersection:
            kind = EShLanguage::EShLangIntersect;
            break;
        case Match::ShaderStage::eCompute:
            kind = EShLanguage::EShLangCompute;
            break;
        }
        glslang::TShader shader(kind);
        auto code_ptr = code.data();
        shader.setStrings(&code_ptr, 1);
        shader.setEnvInput(glslang::EShSourceGlsl, kind, glslang::EShClientVulkan, 450);
        shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
        if (!shader.parse(GetDefaultResources(), 450, ENoProfile, true, false, EShMessages::EShMsgDefault)) {
            MCH_ERROR("{}", shader.getInfoLog())
            MCH_ERROR("{}", shader.getInfoDebugLog())
            return;
        }
        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(EShMessages::EShMsgDefault)) {
            MCH_ERROR("{}", shader.getInfoLog())
            MCH_ERROR("{}", shader.getInfoDebugLog())
            return;
        }
        const auto intermediate = program.getIntermediate(kind);

        std::vector<uint32_t> spirv;
        glslang::GlslangToSpv(*intermediate, spirv);
        create(spirv.data(), spirv.size() * sizeof(uint32_t));
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
