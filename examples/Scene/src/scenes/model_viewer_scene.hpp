#pragma once

#include "scene.hpp"
#include "../camera.hpp"

struct InstanceInfo {
    glm::vec3 offset;
    float scale;
};

struct ModelInfo {
    std::shared_ptr<Match::Model> model;
    std::shared_ptr<Match::VertexBuffer> instances_vertex_buffer;
    std::vector<InstanceInfo> instances_info;
    //       MeshName    是否可见
    std::map<std::string, bool> meshes_visibale;
};

class ModelViewerScene : public Scene {
    define_scene(ModelViewerScene)
private:
    std::unique_ptr<Camera> camera;
    std::shared_ptr<Match::GraphicsShaderProgram> shader_program;
    std::shared_ptr<Match::VertexBuffer> vertex_buffer;
    std::shared_ptr<Match::IndexBuffer> index_buffer;
    // 创建模型时通过模型文件名，找到对应的模型文件信息并添加一个实例
    //       model filename      model
    std::map<std::string, ModelInfo> model_infos;

    // 根据实例名查找实例属性时，通过实例名找到模型文件名，通过模型文件名，找到对应的模型文件信息，最后通过instance index找到对应的instance info
    //       instance name          model filename    instance index
    std::map<std::string, std::pair<std::string, uint32_t>> instance_querys;
};
