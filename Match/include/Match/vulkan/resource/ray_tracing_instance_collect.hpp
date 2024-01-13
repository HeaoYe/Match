#pragma once

#include <Match/vulkan/resource/model.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    
    class RayTracingInstanceCollect {
        no_copy_move_construction(RayTracingInstanceCollect)
    INNER_VISIBLE:
        struct CustomInstanceInfoBuffer {
            using ReleaseCallback = std::function<void()>;
            using EmplaceCallback = std::function<void(uint32_t)>;
            using CreateBufferCallback = std::function<void()>;
            void *stage_vector_address;
            ReleaseCallback release_callback;
            EmplaceCallback emplace_callback;
            CreateBufferCallback create_buffer_callback;
            std::shared_ptr<Buffer> buffer;
        };
        struct InstanceAddressInfo {
            vk::DeviceAddress vertex_buffer_address;
            vk::DeviceAddress index_buffer_address;
        };
        struct GroupInfo {
            uint32_t instance_offset;
            uint32_t instance_count;
            std::vector<std::pair<std::shared_ptr<Model>, glm::mat4>> instance_create_infos;
        };
        using UpdateBatchCallback = std::function<void(uint32_t, uint32_t, uint32_t)>;
        using InterfaceSetTransform = std::function<void(const glm::mat4 &)>;
        using UpdateCallback = std::function<void(uint32_t, InterfaceSetTransform)>;
        template <class CustomInstanceInfo>
        using UpdateCustomInfoCallback = std::function<void(uint32_t, CustomInstanceInfo &)>;
    public:
        RayTracingInstanceCollect(bool allow_update);
        template <class CustomInstanceInfo>
        RayTracingInstanceCollect &register_custom_instance_info();
        template <class ...Args>
        RayTracingInstanceCollect &add_instance(uint32_t group, std::shared_ptr<Model> model, const glm::mat4 &transform, Args &&... args);
        template <class CustomInstanceInfo>
        std::shared_ptr<Buffer> get_custom_data_buffer();
        std::shared_ptr<Buffer> get_instance_address_info_buffer() {
            return get_custom_data_buffer<InstanceAddressInfo>();
        }
        RayTracingInstanceCollect &build();
        RayTracingInstanceCollect &update(uint32_t group, UpdateCallback update_callback);
        template <class CustomInstanceInfo>
        RayTracingInstanceCollect &update(uint32_t group, UpdateCustomInfoCallback<CustomInstanceInfo> update_callback);
        ~RayTracingInstanceCollect();
    INNER_VISIBLE:
        template <class T, class ...Args>
        uint32_t add_instance_custom_info(T && arg, Args &&...args);
        uint32_t add_instance_address_info(Model &model);
        bool check_model_suitable(Model &model);
        void multithread_update(uint32_t group, UpdateBatchCallback update_batch_callback);
    INNER_VISIBLE:
        bool allow_update;

        std::unique_ptr<Buffer> scratch_buffer;
        std::unique_ptr<Buffer> acceleration_struction_instance_infos_buffer;
        std::map<ClassHashCode, CustomInstanceInfoBuffer> instance_infos_buffer_map;

        vk::AccelerationStructureKHR instance_collect;
        std::shared_ptr<Buffer> instance_collect_buffer;
        
        std::map<uint32_t, GroupInfo> groups;
    };

    template <class CustomInstanceInfo>
    RayTracingInstanceCollect &RayTracingInstanceCollect::register_custom_instance_info() {
        if (instance_infos_buffer_map.find(get_class_hash_code<CustomInstanceInfo>()) != instance_infos_buffer_map.end()) {
            MCH_WARN("You have registered the type {}", typeid(std::remove_reference_t<CustomInstanceInfo>).name())
            return *this;
        }
        std::vector<std::remove_reference_t<CustomInstanceInfo>> *stage_vector = new std::vector<std::remove_reference_t<CustomInstanceInfo>>();
        instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()] = {
            .stage_vector_address = static_cast<void *>(stage_vector),
            .release_callback = [this]() {
                auto *stage_vector_ptr = static_cast<std::vector<std::remove_reference_t<CustomInstanceInfo>> *>(instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()].stage_vector_address);
                stage_vector_ptr->clear();
                delete stage_vector_ptr;
            },
            .emplace_callback = [this](uint32_t size) {
                auto *stage_vector_ptr = static_cast<std::vector<std::remove_reference_t<CustomInstanceInfo>> *>(instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()].stage_vector_address);
                while (stage_vector_ptr->size() < size) {
                    stage_vector_ptr->emplace_back();
                }
            },
            .create_buffer_callback = [this]() {
                auto &buffer_info = instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()];
                auto *stage_vector_ptr = static_cast<std::vector<std::remove_reference_t<CustomInstanceInfo>> *>(buffer_info.stage_vector_address);
                size_t size = stage_vector_ptr->size() * sizeof(std::remove_reference_t<CustomInstanceInfo>);
                buffer_info.buffer = std::make_shared<Buffer>(size, vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
                auto ptr = buffer_info.buffer->map();
                memcpy(ptr, stage_vector_ptr->data(), size);
                if (!allow_update) {
                    buffer_info.buffer->unmap();
                }
            },
            .buffer = nullptr,
        };
        return *this;
    }

    template <class ...Args>
    RayTracingInstanceCollect &RayTracingInstanceCollect::add_instance(uint32_t group, std::shared_ptr<Model> model, const glm::mat4 &transform, Args &&... args) {
        if (!check_model_suitable(*model)) {
            return *this;
        }

        if (groups.find(group) == groups.end()) {
            groups[group] = {
                .instance_offset = 0,
                .instance_count = 0,
                .instance_create_infos = {},
            };
        }
        auto &group_info = groups.at(group);
        group_info.instance_create_infos.push_back(std::make_pair(model, transform));

        if constexpr (sizeof...(args) > 0) {
            add_instance_custom_info<Args...>(std::forward<Args>(args)...);
        }
        uint32_t size = add_instance_address_info(*model);

        for (auto &[hash_code, buffer] : instance_infos_buffer_map) {
            buffer.emplace_callback(size);
        }
        return *this;
    }

    template <class T, class ...Args>
    uint32_t RayTracingInstanceCollect::add_instance_custom_info(T && arg, Args &&...args) {
        auto *stage_vector_ptr = static_cast<std::vector<std::remove_reference_t<T>> *>(instance_infos_buffer_map[get_class_hash_code<T>()].stage_vector_address);
        stage_vector_ptr->push_back(std::forward<std::remove_reference_t<T>>(arg));
        if constexpr (sizeof...(args) > 0) {
            return add_instance_custom_info<Args...>(std::forward<Args>(args)...);
        } else {
            return stage_vector_ptr->size();
        }
    }

    template <class CustomInstanceInfo>
    std::shared_ptr<Buffer> RayTracingInstanceCollect::get_custom_data_buffer() {
        return instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()].buffer;
    }

    template <class CustomInstanceInfo>
    RayTracingInstanceCollect &RayTracingInstanceCollect::update(uint32_t group, UpdateCustomInfoCallback<CustomInstanceInfo> update_callback) {
        multithread_update(group, [&](uint32_t in_group_index, uint32_t batch_begin, uint32_t batch_end) {
            auto *custom_info_ptr = static_cast<std::remove_reference_t<CustomInstanceInfo> *>(instance_infos_buffer_map[get_class_hash_code<CustomInstanceInfo>()].buffer->map());
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
