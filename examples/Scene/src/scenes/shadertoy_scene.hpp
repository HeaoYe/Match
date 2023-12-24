#pragma once

#include "scene.hpp"

class ShaderToyInput {
public:
    ShaderToyInput(Match::ResourceFactory &factory);
public:
    struct ShaderToyInputUniform {
        alignas(16) glm::vec3 iResolution;
        alignas(4) float iTime;
        alignas(4) float iTimeDelta;
        alignas(4) float iFrameRate;
        alignas(4) float iFrame;
        alignas(16) glm::vec4 iMouse;
        alignas(16) glm::vec4 iDate;
    } inner;
    ShaderToyInputUniform *uniform;
    std::shared_ptr<Match::UniformBuffer> uniform_buffer;
};

class ShaderToyScene : public Scene {
    define_scene(ShaderToyScene)
private:
    void updata_shader_program();
    float time = 0;
    std::unique_ptr<ShaderToyInput> shader_input;
    char shader_file_name[1024] = "editor.frag";
    std::shared_ptr<Match::Shader> vert_shader, frag_shader;
    std::shared_ptr<Match::VertexAttributeSet> vas;
    std::shared_ptr<Match::ShaderProgram> shader_program;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
