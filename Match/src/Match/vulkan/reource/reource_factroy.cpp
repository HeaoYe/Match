#include <Match/vulkan/resource/resource_factory.hpp>
#include <Match/vulkan/descriptor_resource/spec_texture.hpp>
#include <Match/core/utils.hpp>
#include <shaderc/shaderc.hpp>

namespace Match {
    ResourceFactory::ResourceFactory(const std::string &root) : root(root) {
    }

    std::shared_ptr<RenderPassBuilder> ResourceFactory::create_render_pass_builder() {
        return std::make_shared<RenderPassBuilder>();
    }

    std::shared_ptr<Renderer> ResourceFactory::create_renderer(std::shared_ptr<RenderPassBuilder> builder) {
        return std::make_shared<Renderer>(builder);
    }

    std::shared_ptr<Shader> ResourceFactory::load_shader(const std::string &filename, ShaderType type) {
        auto code = read_binary_file(root + "/shaders/" + filename);

        if (type != ShaderType::eCompiled) {
            return std::make_shared<Shader>(filename, std::string(code.data(), code.size()), type);
        }
        return std::make_shared<Shader>(code);
    }

    std::shared_ptr<Shader> ResourceFactory::load_shader_from_string(const std::string &code, ShaderType type) {
        return std::make_shared<Shader>("string code", code, type);
    }

    std::shared_ptr<VertexAttributeSet> ResourceFactory::create_vertex_attribute_set(const std::vector<InputBindingInfo> &binding_infos) {
        return std::make_shared<VertexAttributeSet>(binding_infos);
    }
    
    std::shared_ptr<ShaderProgram> ResourceFactory::create_shader_program(std::weak_ptr<Renderer> renderer, const std::string &subpass_name) {
        return std::make_shared<ShaderProgram>(renderer, subpass_name);
    }

    std::shared_ptr<VertexBuffer> ResourceFactory::create_vertex_buffer(uint32_t vertex_size, uint32_t count) {
        return std::make_shared<VertexBuffer>(vertex_size, count);
    }

    std::shared_ptr<IndexBuffer> ResourceFactory::create_index_buffer(IndexType type, uint32_t count) {
        return std::make_shared<IndexBuffer>(type, count);
    }

    std::shared_ptr<Sampler> ResourceFactory::create_sampler(const SamplerOptions &options) {
        return std::make_shared<Sampler>(options);
    }

    std::shared_ptr<UniformBuffer> ResourceFactory::create_uniform_buffer(uint32_t size) {
        return std::make_shared<UniformBuffer>(size);
    }

    std::shared_ptr<Texture> ResourceFactory::load_texture(const std::string &filename, uint32_t mip_levels) {
        auto pos = filename.find_last_of('.') + 1;
        auto filetype = filename.substr(pos, filename.size() - pos);
        std::string path = root + "/textures/" + filename;
        if (filetype == "jpg") {
            return std::make_shared<ImgTexture>(path, mip_levels);
        }
        if (filetype == "ktx") {
            if (mip_levels != 0) {
                MCH_WARN("Ktx Texture doesn't support custom mip_levels")
            }
            return std::make_shared<KtxTexture>(path);
        }
        MCH_ERROR("Unsupported texture format .{}", filetype);
        return nullptr;
    }

    std::shared_ptr<Texture> ResourceFactory::create_texture(const uint8_t *data, uint32_t width, uint32_t height, uint32_t mip_levels) {
        return std::make_shared<DataTexture>(data, width, height, mip_levels);
    }
}
