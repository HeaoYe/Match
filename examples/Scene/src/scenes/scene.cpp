#include "scene.hpp"
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <glm/gtx/rotate_vector.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <tiny_obj_loader.h>

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.in_pos) ^
                   (hash<glm::vec3>()(vertex.in_normal) << 1)) >> 1) ^
                   (hash<glm::vec3>()(vertex.in_color) << 1);
        }
    };
}

void Scene::load_model(const std::string &filename) {
    auto path = "resource/models/" + filename;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> unique_vertices;
    uint32_t count = 0;
    for (const auto& shape : shapes) {
        count += shape.mesh.indices.size();
    }
    vertices.reserve(count);
    indices.reserve(count);

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex {};
            vertex.in_pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };
            vertex.in_normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            vertex.in_color = { 1, 1, 1 };

            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(unique_vertices[vertex]);
        }
    }
}

Scene::Scene(std::shared_ptr<Match::ResourceFactory> factory, std::shared_ptr<Match::Renderer> renderer) : factory(factory), renderer(renderer) {
}

Scene::~Scene() {
    indices.clear();
    vertices.clear();
}

void SceneManager::update(float delta) {
    if (current_scene.get() == nullptr) {
        return;
    }
    current_scene->update(delta);
}

void SceneManager::render() {
    if (current_scene.get() == nullptr) {
        return;
    }
    current_scene->render();
}

void SceneManager::render_imgui() {
    if (current_scene.get() == nullptr) {
        return;
    }
    current_scene->render_imgui();
}

SceneManager::~SceneManager() {
    if (current_scene.get() != nullptr) {
        current_scene->destroy();
        current_scene.reset();
    }
    renderer.reset();
    factory.reset();
}
