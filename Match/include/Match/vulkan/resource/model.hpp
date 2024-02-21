#pragma once

#include <Match/vulkan/resource/buffer.hpp>
#include <Match/vulkan/resource/vertex_attribute_set.hpp>
#include <Match/vulkan/resource/model_acceleration_structure.hpp>
#include <Match/vulkan/resource/custom_data_registrar.hpp>
#include <optional>

namespace Match {
    struct MATCH_API Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 color;

        bool operator==(const Vertex& rhs) const {
            return pos == rhs.pos && normal == rhs.normal && color == rhs.color;
        }
        static InputBindingInfo generate_input_binding(uint32_t binding);
    };

    struct MATCH_API BufferPosition {
        uint32_t vertex_buffer_offset;
        uint32_t index_buffer_offset;
    };

    class MATCH_API Mesh {
        no_copy_move_construction(Mesh)
    public:
        Mesh();
        ~Mesh();
        uint32_t get_index_count() const { return indices.size(); }
    INNER_VISIBLE:
        BufferPosition position;
        std::vector<uint32_t> indices;
    };

    class MATCH_API RayTracingModel {
        default_no_copy_move_construction(RayTracingModel)
    public:
        enum class RayTracingModelType {
            eModel,
            eSphereCollect,
            eGLTFPrimitive,
        };
    public:
        virtual RayTracingModelType get_ray_tracing_model_type() = 0;
        virtual ~RayTracingModel();
    INNER_PROTECT:
        std::optional<std::unique_ptr<ModelAccelerationStructure>> acceleration_structure;
    };

    class MATCH_API RayTracingScene {
        default_no_copy_move_construction(RayTracingScene)
    public:
        enum class RayTracingSceneType {
            eGLTFScene,
        };
    INNER_PROTECT:
    public:
        virtual RayTracingSceneType get_ray_tracing_scene_type() = 0;
        virtual ~RayTracingScene() = default;
    };

    class MATCH_API Model final : public RayTracingModel {
        no_copy_move_construction(Model)
    public:
        Model(const std::string &filename);
        BufferPosition upload_data(std::shared_ptr<VertexBuffer> vertex_buffer, std::shared_ptr<IndexBuffer> index_buffer, BufferPosition position = { 0, 0 });
        std::shared_ptr<const Mesh> get_mesh_by_name(const std::string &name) const;
        std::vector<std::string> enumerate_meshes_name() const;
        uint32_t get_vertex_count() const { return vertex_count; }
        uint32_t get_index_count() const { return index_count; }
        RayTracingModelType get_ray_tracing_model_type() override { return RayTracingModelType::eModel; }
        ~Model() override;
    INNER_VISIBLE:
        BufferPosition position;
        std::vector<Vertex> vertices;
        uint32_t vertex_count;
        uint32_t index_count;
        std::map<std::string, std::shared_ptr<Mesh>> meshes;
    INNER_VISIBLE:
        // using for ray_tracing_acceration_structure
        std::unique_ptr<Buffer> vertex_buffer;
        std::unique_ptr<Buffer> index_buffer;
    };

    struct MATCH_API SphereData {
        glm::vec3 center;
        float radius;
    };

    class MATCH_API SphereCollect final : public RayTracingModel {
        no_copy_move_construction(SphereCollect)
    INNER_VISIBLE:
        struct SphereAaBbData {
            glm::vec3 minium;
            glm::vec3 maxium;
        };
        template <class CustomSphereDatas>
        using UpdateCustomDataCallback = std::function<void(uint32_t, CustomSphereDatas &)>;
    public:
        SphereCollect();
        template <class CustomSphereData>
        SphereCollect &register_custom_sphere_data();
        template <class ...CustomSphereDatas>
        SphereCollect &add_sphere(uint32_t group_id, glm::vec3 center, float radius, CustomSphereDatas &&... datas);
        SphereCollect &build();
        SphereCollect &update(uint32_t group_id, UpdateCustomDataCallback<SphereData> update_callback);
        template <class CustomSphereData>
        SphereCollect &update(uint32_t group_id, UpdateCustomDataCallback<CustomSphereData> update_callback);
        template <class CustomSphereData>
        std::shared_ptr<Buffer> get_custom_data_buffer() { return registrar->get_custom_data_buffer<CustomSphereData>(); }
        std::shared_ptr<Buffer> get_spheres_buffer() { return registrar->get_custom_data_buffer<SphereData>(); };
        RayTracingModelType get_ray_tracing_model_type() override { return RayTracingModelType::eSphereCollect; }
        ~SphereCollect() override;
    INNER_VISIBLE:
        std::unique_ptr<CustomDataRegistrar<uint8_t>> registrar;
        std::unique_ptr<AccelerationStructureBuilder> builder;
        std::vector<SphereAaBbData> aabbs;
        std::shared_ptr<Buffer> aabbs_buffer;
    };

    template <class CustomSphereData>
    SphereCollect &SphereCollect::register_custom_sphere_data() {
        registrar->register_custom_data<CustomSphereData>();
        return *this;
    }

    template <class ...CustomSphereDatas>
    SphereCollect &SphereCollect::add_sphere(uint32_t group_id, glm::vec3 center, float radius, CustomSphereDatas &&... datas) {
        registrar->add_instance(
            group_id,
            0,
            SphereData {
                .center = center,
                .radius = radius,
            },
            std::forward<CustomSphereDatas>(datas)...
        );
        return *this;
    }

    template <class CustomSphereData>
    SphereCollect &SphereCollect::update(uint32_t group_id, UpdateCustomDataCallback<CustomSphereData> update_callback) {
        registrar->multithread_update(group_id, [=](uint32_t in_group_index, uint32_t batch_begin, uint32_t batch_end) {
            auto *sphere_data_ptr = static_cast<SphereData *>(registrar->get_custom_data_buffer<SphereData>()->map());
            sphere_data_ptr += batch_begin;
            while (batch_begin < batch_end) {
                update_callback(in_group_index, *sphere_data_ptr);
                sphere_data_ptr ++;
                in_group_index ++;
                batch_begin ++;
            }
        });
        return *this;
    }
}
