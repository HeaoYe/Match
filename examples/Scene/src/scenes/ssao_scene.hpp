#pragma once

#include "scene.hpp"
#include "../camera.hpp"

class SSAOScene : public Scene {
    define_scene(SSAOScene)
private:
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Match::UniformBuffer> ssao_samples_uniform_buffer;
    std::shared_ptr<Match::Texture> ssao_random_vecs_texture;
    // ShaderProgram改名为GraphicsShaderProgram，为之后的RayTracingShaderProgram做准备
    std::shared_ptr<Match::GraphicsShaderProgram> prepare_shader_program;
    std::shared_ptr<Match::GraphicsShaderProgram> ssao_shader_program;
    std::shared_ptr<Match::GraphicsShaderProgram> main_shader_program;
    // PushConstants
    std::shared_ptr<Match::PushConstants> ssao_shader_program_constants;
    std::shared_ptr<Match::PushConstants> main_shader_program_constants;
    std::shared_ptr<Match::Model> model;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
};
