#pragma once

#include <Match/vulkan/resource/model.hpp>
#include <Match/vulkan/resource/gltf_scene.hpp>
#include <Match/vulkan/resource/custom_data_registrar.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    class MATCH_API RayTracingInstanceCollect {
        no_copy_move_construction(RayTracingInstanceCollect)
    INNER_VISIBLE:
        struct InstanceCreateInfo {
            std::shared_ptr<RayTracingModel> model;
            glm::mat4 transform_matrix;
            uint32_t hit_group;
        };
        struct InstanceAddressData {
            vk::DeviceAddress vertex_buffer_address;
            vk::DeviceAddress index_buffer_address;
        };

        using InterfaceSetTransform = std::function<void(const glm::mat4 &)>;
        using InterfaceUpdataAccelerationStructure = std::function<void(std::shared_ptr<RayTracingModel>)>;
        using UpdateCallback = std::function<void(uint32_t, InterfaceSetTransform, InterfaceUpdataAccelerationStructure)>;
        template <class CustomInstanceData>
        using UpdateCustomDataCallback = std::function<void(uint32_t, CustomInstanceData &)>;
    public:
        RayTracingInstanceCollect(bool allow_update);
        template <class CustomInstanceData>
        RayTracingInstanceCollect &register_custom_instance_data();
        RayTracingInstanceCollect &register_gltf_primitive_instance_data() {
            return register_custom_instance_data<GLTFPrimitiveInstanceData>();
        }
        template <class ...Args>
        RayTracingInstanceCollect &add_instance(uint32_t group_id, std::shared_ptr<RayTracingModel> model, const glm::mat4 &transform_matrix, uint32_t hit_group, Args &&... args);
        template <class ...Args>
        RayTracingInstanceCollect &add_scene(uint32_t group_id, std::shared_ptr<RayTracingScene> scene, uint32_t hit_group, Args &&... args);
        template <class CustomInstanceData>
        std::shared_ptr<Buffer> get_custom_instance_data_buffer();
        std::shared_ptr<Buffer> get_instance_address_data_buffer() { return get_custom_instance_data_buffer<InstanceAddressData>(); }
        std::shared_ptr<Buffer> get_gltf_primitive_data_buffer() { return get_custom_instance_data_buffer<GLTFPrimitiveInstanceData>(); }
        RayTracingInstanceCollect &build();
        RayTracingInstanceCollect &update(uint32_t group_id, UpdateCallback update_callback);
        template <class CustomInstanceData>
        RayTracingInstanceCollect &update(uint32_t group_id, UpdateCustomDataCallback<CustomInstanceData> update_callback);
        ~RayTracingInstanceCollect();
    INNER_VISIBLE:
        InstanceAddressData create_instance_address_data(RayTracingModel &model);
        bool check_model_suitable(RayTracingModel &model);
    INNER_VISIBLE:
        bool allow_update;

        std::unique_ptr<CustomDataRegistrar<InstanceCreateInfo>> registrar;
        uint32_t instance_count;

        std::unique_ptr<Buffer> scratch_buffer;
        std::unique_ptr<Buffer> acceleration_struction_instance_infos_buffer;

        vk::AccelerationStructureKHR instance_collect;
        std::shared_ptr<Buffer> instance_collect_buffer;
    };

    template <class CustomInstanceData>
    RayTracingInstanceCollect &RayTracingInstanceCollect::register_custom_instance_data() {
        registrar->register_custom_data<CustomInstanceData>();
        return *this;
    }

    template <class ...Args>
    RayTracingInstanceCollect &RayTracingInstanceCollect::add_instance(uint32_t group_id, std::shared_ptr<RayTracingModel> model, const glm::mat4 &transform_matrix, uint32_t hit_group, Args &&... args) {
        if (!check_model_suitable(*model)) {
            return *this;
        }

        registrar->add_instance(
            group_id,
            {
                .model = model,
                .transform_matrix = transform_matrix,
                .hit_group = hit_group
            },
            create_instance_address_data(*model),
            std::forward<Args>(args)...
        );

        return *this;
    }

    template <class ...Args>
    RayTracingInstanceCollect &RayTracingInstanceCollect::add_scene(uint32_t group_id, std::shared_ptr<RayTracingScene> scene, uint32_t hit_group, Args &&... args) {
        switch(scene->get_ray_tracing_scene_type()) {
        case RayTracingScene::RayTracingSceneType::eGLTFScene:
            auto gltf_scene = std::dynamic_pointer_cast<GLTFScene>(scene);
            gltf_scene->enumerate_primitives([&](auto *node, auto gltf_primitive) {
                add_instance(group_id, gltf_primitive, node->get_world_matrix(), hit_group, gltf_primitive->get_primitive_instance_data(), std::forward<Args>(args)...);
            });
            return *this;
        }
        return *this;
    }

    template <class CustomInstanceData>
    std::shared_ptr<Buffer> RayTracingInstanceCollect::get_custom_instance_data_buffer() {
        return registrar->get_custom_data_buffer<CustomInstanceData>();
    }

    template <class CustomInstanceData>
    RayTracingInstanceCollect &RayTracingInstanceCollect::update(uint32_t group_id, UpdateCustomDataCallback<CustomInstanceData> update_callback) {
        registrar->multithread_update(group_id, [=](uint32_t in_group_index, uint32_t batch_begin, uint32_t batch_end) {
            auto *custom_info_ptr = static_cast<std::remove_reference_t<CustomInstanceData> *>(get_custom_instance_data_buffer<CustomInstanceData>()->map());
            custom_info_ptr += batch_begin;
            while (batch_begin < batch_end) {
                update_callback(in_group_index, *custom_info_ptr);
                custom_info_ptr ++;
                in_group_index ++;
                batch_begin ++;
            }
        });
        return *this;
    }
}
