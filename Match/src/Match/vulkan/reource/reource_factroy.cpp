#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/core/utils.hpp>
#include <shaderc/shaderc.hpp>

namespace Match {
    ResourceFactory::ResourceFactory(const std::string &root) : root(root) {
    }

    std::shared_ptr<Shader> ResourceFactory::load_shader(const std::string &filename, ShaderType type) {
        auto code = read_binary_file(root + "/shaders/" + filename);

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
            auto module = compiler.CompileGlslToSpv(code.data(), code.size(), kind, filename.c_str(), {});
            if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
                MCH_ERROR("Compile {} faild: {}", filename, module.GetErrorMessage());
            }
            std::vector<uint32_t> spirv(module.cbegin(), module.cend());
            return std::make_shared<Shader>(spirv);
        }
        return std::make_shared<Shader>(code);
    }

    std::shared_ptr<VertexAttributeSet> ResourceFactory::create_vertex_attribute_set() {
        return std::make_shared<VertexAttributeSet>();
    }
    
    std::shared_ptr<ShaderProgram> ResourceFactory::create_shader_program(const std::string &subpass_name) {
        return std::make_shared<ShaderProgram>(subpass_name);
    }

    std::shared_ptr<VertexBuffer> ResourceFactory::create_vertex_buffer(uint32_t vertex_size, uint32_t count) {
        return std::make_shared<VertexBuffer>(vertex_size, count);
    }

    std::shared_ptr<IndexBuffer> ResourceFactory::create_index_buffer(IndexType type, uint32_t count) {
        return std::make_shared<IndexBuffer>(type, count);
    }
}
