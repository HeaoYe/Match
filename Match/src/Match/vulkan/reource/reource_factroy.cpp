#include <Match/vulkan/resource/resource_factory.hpp>

namespace Match {
    ResourceFactory::ResourceFactory(const std::string &root) : root(root) {
    }

    std::shared_ptr<Shader> ResourceFactory::load_shader(const std::string &filename) {
        return std::make_shared<Shader>(root + "/shaders/" + filename);
    }
    
    std::shared_ptr<ShaderProgram> ResourceFactory::create_shader_program(const std::string &subpass_name) {
        return std::make_shared<ShaderProgram>(subpass_name);
    }
}
