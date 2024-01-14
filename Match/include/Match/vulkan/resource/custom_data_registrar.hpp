#pragma once

#include <Match/vulkan/resource/buffer.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    template <class CustomCreateInfo>
    class CustomDataRegistrar {
        default_no_copy_move_construction(CustomDataRegistrar)
    INNER_VISIBLE:
        struct CustomDataWrapper {
            uint32_t size;
            std::function<void *()> create_stage_vector_callback;
            std::function<void(void *, uint32_t)> align_stage_vector_size_callback;
            std::function<void *(void *)> get_stage_vector_data_ptr_callback;
            std::function<uint32_t(void *)> get_stage_vector_size_callback;
            std::function<std::shared_ptr<Buffer>(uint32_t)> create_buffer_callback;
            std::function<void(void *)> destroy_stage_vector_callback;
        };

        struct GroupInfo {
            uint32_t offset;
            uint32_t count;
            std::vector<CustomCreateInfo> custom_create_infos;
            std::map<ClassHashCode, void *> custom_data_stage_vector_ptr_map;
        };

        using UpdateBatchCallback = std::function<void(uint32_t, uint32_t, uint32_t)>;
    public:
        template <class CustomData>
        void register_custom_data()  {
            auto class_hash_code = get_class_hash_code<CustomData>();
            using type_t = std::remove_reference_t<CustomData>;
            using vector_t = std::vector<type_t>;
            if (custom_data_wrapper_map.find(class_hash_code) != custom_data_wrapper_map.end()) {
                MCH_WARN("The type {} has been registered", typeid(std::remove_reference_t<CustomData>).name())
                return;
            }
            custom_data_wrapper_map[class_hash_code] = {
                .size = sizeof(type_t),
                .create_stage_vector_callback = []() {
                    auto *stage_vector_ptr = new vector_t();
                    return stage_vector_ptr;
                },
                .align_stage_vector_size_callback = [](void *ptr, uint32_t size) {
                    auto *stage_vector_ptr = static_cast<vector_t *>(ptr);
                    while (stage_vector_ptr->size() < size) {
                        stage_vector_ptr->emplace_back();
                    }
                },
                .get_stage_vector_data_ptr_callback = [](void *ptr) {
                    auto *stage_vector_ptr = static_cast<vector_t *>(ptr);
                    return stage_vector_ptr->data();
                },
                .get_stage_vector_size_callback = [](void *ptr) {
                    auto *stage_vector_ptr = static_cast<vector_t *>(ptr);
                    return stage_vector_ptr->size();
                },
                .create_buffer_callback = [](uint32_t count) {
                    return std::make_shared<Buffer>(count * sizeof(type_t), vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
                },
                .destroy_stage_vector_callback = [](void *ptr) {
                    auto *stage_vector_ptr = static_cast<vector_t *>(ptr);
                    stage_vector_ptr->clear();
                    delete stage_vector_ptr;
                },
            };
        }

        template <class ...CustomDatas>
        void add_instance(uint32_t group_id, const CustomCreateInfo &custom_create_info, CustomDatas &&... datas) {
            if (groups.find(group_id) == groups.end()) {
                groups.insert(std::make_pair(group_id, create_group()));
            }

            auto &group_info = groups.at(group_id);
            group_info.custom_create_infos.push_back(custom_create_info);

            if constexpr (sizeof...(datas) > 0) {
                add_custom_data<CustomDatas...>(group_info, std::forward<CustomDatas>(datas)...);
            }

            for (auto &[class_hash_code, ptr] : group_info.custom_data_stage_vector_ptr_map) {
                custom_data_wrapper_map.at(class_hash_code).align_stage_vector_size_callback(ptr, group_info.custom_create_infos.size());
            }
        }

        template<class CustomData>
        std::shared_ptr<Buffer> get_custom_data_buffer() {
            return custom_data_buffer_map.at(get_class_hash_code<CustomData>());
        }

        uint32_t build_groups() {
            uint32_t count = 0;
            for (auto &[group_id, group_info] : groups) {
                auto [class_hash_code, ptr] = *group_info.custom_data_stage_vector_ptr_map.begin();
                group_info.offset = count;
                group_info.count = custom_data_wrapper_map.at(class_hash_code).get_stage_vector_size_callback(ptr);
                count += group_info.count;
            }

            for (auto &[class_hash_code, wrapper] : custom_data_wrapper_map) {
                custom_data_buffer_map.insert(std::make_pair(class_hash_code, wrapper.create_buffer_callback(count)));
            }

            for (auto &[group_id, group_info] : groups) {
                for (auto &[class_hash_code, ptr] : group_info.custom_data_stage_vector_ptr_map) {
                    auto &wrapper = custom_data_wrapper_map.at(class_hash_code);
                    memcpy(
                        static_cast<uint8_t *>(custom_data_buffer_map.at(class_hash_code)->map()) + wrapper.size * group_info.offset,
                        wrapper.get_stage_vector_data_ptr_callback(ptr),
                        wrapper.get_stage_vector_size_callback(ptr) * wrapper.size
                    );
                    custom_data_buffer_map.at(class_hash_code)->unmap();
                }
            }

            clear_groups();
            
            return count;
        }

        void multithread_update(uint32_t group_id, UpdateBatchCallback update_batch_callback) {
            auto &group_info = groups.at(group_id);
            uint32_t begin = group_info.offset;
            uint32_t end = begin + group_info.count;

            std::vector<std::thread> thread_pool;
            uint32_t in_group_index = 0;
            while (begin < end) {
                uint32_t batch_end = std::min(begin + 500, end);
                thread_pool.emplace_back([=]() {
                    update_batch_callback(in_group_index, begin, batch_end);
                });
                in_group_index += 500;
                begin = batch_end;
            }
            for (auto &thread : thread_pool) {
                thread.join();
            }
        }

        ~CustomDataRegistrar() {
            custom_data_wrapper_map.clear();
            clear_groups();
            groups.clear();
            custom_data_buffer_map.clear();
        }
    INNER_VISIBLE:
        GroupInfo create_group() {
            std::map<ClassHashCode, void *> custom_data_stage_vector_ptr_map;
            for (auto &[class_hash_code, wrapper] : custom_data_wrapper_map) {
                custom_data_stage_vector_ptr_map.insert(std::make_pair(class_hash_code, wrapper.create_stage_vector_callback()));
            }
            return {
                .offset = 0,
                .count = 0,
                .custom_create_infos = {},
                .custom_data_stage_vector_ptr_map = std::move(custom_data_stage_vector_ptr_map),
            };
        }

        void clear_groups() {
            for (auto &[group_id, group_info] : groups) {
                for (auto [class_hash_code, ptr] : group_info.custom_data_stage_vector_ptr_map) {
                    custom_data_wrapper_map.at(class_hash_code).destroy_stage_vector_callback(ptr);
                }
                group_info.custom_data_stage_vector_ptr_map.clear();
            }
        }

        template <class T, class ...Args>
        uint32_t add_custom_data(GroupInfo &group_info, T && arg, Args &&...args) {
            auto *stage_vector_ptr = static_cast<std::vector<std::remove_reference_t<T>> *>(group_info.custom_data_stage_vector_ptr_map[get_class_hash_code<T>()]);
            stage_vector_ptr->push_back(std::forward<std::remove_reference_t<T>>(arg));
            if constexpr (sizeof...(args) > 0) {
                return add_custom_data<Args...>(group_info, std::forward<Args>(args)...);
            } else {
                return stage_vector_ptr->size();
            }
        }
    INNER_VISIBLE:
        std::map<ClassHashCode, CustomDataWrapper> custom_data_wrapper_map;
        std::map<uint32_t, GroupInfo> groups;
        std::map<ClassHashCode, std::shared_ptr<Buffer>> custom_data_buffer_map;
    };
}
