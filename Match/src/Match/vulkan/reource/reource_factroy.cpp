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

    std::shared_ptr<Shader> ResourceFactory::load_shader(const std::string &filename) {
        auto code = read_binary_file(root + "/shaders/" + filename);
        return std::make_shared<Shader>(code);
    }

    std::shared_ptr<Shader> ResourceFactory::compile_shader(const std::string &filename, ShaderStage stage) {
        auto code = read_binary_file(root + "/shaders/" + filename);
        return std::make_shared<Shader>(filename, std::string(code.data(), code.size()), stage);
    }

    std::shared_ptr<Shader> ResourceFactory::compile_shader_from_string(const std::string &code, ShaderStage stage) {
        return std::make_shared<Shader>("string code", code, stage);
    }

    std::shared_ptr<VertexAttributeSet> ResourceFactory::create_vertex_attribute_set(const std::vector<InputBindingInfo> &binding_infos) {
        return std::make_shared<VertexAttributeSet>(binding_infos);
    }
    
    std::shared_ptr<GraphicsShaderProgram> ResourceFactory::create_shader_program(std::weak_ptr<Renderer> renderer, const std::string &subpass_name) {
        return std::make_shared<GraphicsShaderProgram>(renderer, subpass_name);
    }

    std::shared_ptr<VertexBuffer> ResourceFactory::create_vertex_buffer(uint32_t vertex_size, uint32_t count, vk::BufferUsageFlags additional_usage) {
        return std::make_shared<VertexBuffer>(vertex_size, count, additional_usage);
    }

    std::shared_ptr<IndexBuffer> ResourceFactory::create_index_buffer(IndexType type, uint32_t count, vk::BufferUsageFlags additional_usage) {
        return std::make_shared<IndexBuffer>(type, count, additional_usage);
    }

    std::shared_ptr<DescriptorSet> ResourceFactory::create_descriptor_set(std::weak_ptr<Renderer> renderer) {
        return std::make_shared<DescriptorSet>(renderer);
    }

    std::shared_ptr<Sampler> ResourceFactory::create_sampler(const SamplerOptions &options) {
        return std::make_shared<Sampler>(options);
    }

    std::shared_ptr<PushConstants> ResourceFactory::create_push_constants(ShaderStages stages, const std::vector<PushConstantInfo> &infos) {
        return std::make_shared<PushConstants>(stages, infos);
    }

    std::shared_ptr<UniformBuffer> ResourceFactory::create_uniform_buffer(uint64_t size, bool create_for_each_frame_in_flight) {
        return std::make_shared<UniformBuffer>(size, create_for_each_frame_in_flight);
    }

    std::shared_ptr<StorageImage> ResourceFactory::create_storage_image(uint32_t width, uint32_t height, vk::Format format, bool sampled) {
        return std::make_shared<StorageImage>(width, height, format, sampled);
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

    std::shared_ptr<Model> ResourceFactory::load_model(const std::string &filename) {
        return std::make_shared<Model>(root + "/models/" + filename);
    }

    std::shared_ptr<AccelerationStructureBuilder> ResourceFactory::create_acceleration_structure_builder() {
        return std::make_shared<AccelerationStructureBuilder>();
    }

    std::shared_ptr<RayTracingInstance> ResourceFactory::create_ray_tracing_instance(const std::vector<std::shared_ptr<Model>> &models) {
        return std::make_shared<RayTracingInstance>(models);
    }
 
    std::shared_ptr<RayTracingShaderProgram> ResourceFactory::create_ray_tracing_shader_program() {
        return std::make_shared<RayTracingShaderProgram>();
    }
}
