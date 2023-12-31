#pragma once

#include "scene.hpp"
#include "../camera.hpp"

class SSAOScene : public Scene {
    define_scene(SSAOScene)
private:
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Match::UniformBuffer> ssao_samples_uniform_buffer;
    std::shared_ptr<Match::Texture> ssao_random_vecs_texture;
    std::shared_ptr<Match::ShaderProgram> prepare_shader_program;
    std::shared_ptr<Match::ShaderProgram> ssao_shader_program;
    std::shared_ptr<Match::ShaderProgram> main_shader_program;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
