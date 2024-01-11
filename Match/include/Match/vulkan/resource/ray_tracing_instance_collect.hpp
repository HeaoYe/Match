#pragma once

#include <Match/vulkan/resource/model.hpp>
#include <Match/core/utils.hpp>

namespace Match {
    class RayTracingInstanceCollectBase {
        no_copy_move_construction(RayTracingInstanceCollectBase)
    INNER_PROTECT:
        using update_instance_infos_func = std::function<void(uint32_t, uint32_t)>;
    public:
        RayTracingInstanceCollectBase(bool allow_update);
        std::shared_ptr<TwoStageBuffer> get_instance_infos_bufer() { return instance_infos_buffer; }
        virtual ~RayTracingInstanceCollectBase();
    INNER_PROTECT:
        void build_acceleration_structure();
        void update_acceleration_structure(uint32_t offset, uint32_t count, update_instance_infos_func update_instance_infos);
        virtual void prepare_instance_infos() = 0;
        bool check_model(Model &model);
        std::tuple<vk::DeviceAddress, vk::DeviceAddress, vk::DeviceAddress> get_model_addresses(Model &model);
    INNER_PROTECT:
        bool allow_update;

        std::unique_ptr<Buffer> scratch_buffer;
        std::vector<vk::AccelerationStructureInstanceKHR> acceleration_struction_instance_infos;
        std::unique_ptr<TwoStageBuffer> acceleration_struction_instance_infos_buffer;

        vk::AccelerationStructureKHR instance_collect;
        std::shared_ptr<Buffer> instance_collect_buffer;

        std::shared_ptr<TwoStageBuffer> instance_infos_buffer;
    };

    struct RayTracingInstanceCollectType {
        using interface_update_transform_func = std::function<void(const glm::mat4 &)>;
        using update_func = std::function<void(uint32_t, interface_update_transform_func interface_update_transform)>;
        struct InstanceAddressInfo {
            vk::DeviceAddress vertex_buffer_address;
            vk::DeviceAddress index_buffer_address;
        };
    };

    template <class CustomInstanceInfo, bool is_void>
    struct RayTracingInstanceCollectTrait {};

    #define define_ray_tracing_instance_collect_class(cls) \
    public: \
        cls(bool allow_update) : RayTracingInstanceCollectBase(allow_update) {} \
        cls &build() { \
            build_acceleration_structure(); \
            return *this; \
        } \
        cls &update(uint32_t group, update_func func) { \
            if (!allow_update) { \
                MCH_ERROR("Instance Collect doesn't allow update") \
                return *this; \
            } \
            auto &group_info = group_infos.at(group); \
            update_acceleration_structure(group_info.instance_start, group_info.instance_count, [&](uint32_t batch_begin, uint32_t batch_end) { \
                auto current_acceleration_struction_instance_info = (vk::AccelerationStructureInstanceKHR *)acceleration_struction_instance_infos_buffer->map(); \
                for (uint32_t i = 0; i < batch_begin; i ++) { \
                    current_acceleration_struction_instance_info ++; \
                } \
                interface_update_transform_func interface_update_transform = [&] (const glm::mat4 &transform_) { \
                    current_acceleration_struction_instance_info->setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(transform_)); \
                }; \
                while (batch_begin < batch_end) { \
                    func(batch_begin - group_info.instance_start, interface_update_transform); \
                    current_acceleration_struction_instance_info ++; \
                    batch_begin += 1; \
                } \
            }); \
            return *this; \
        } \

    template <class CustomInstanceInfo>
    struct RayTracingInstanceCollectTrait<CustomInstanceInfo, false> : public RayTracingInstanceCollectType {
        class RayTracingInstanceCollectCustom final : public RayTracingInstanceCollectBase {
            no_copy_move_construction(RayTracingInstanceCollectCustom)
            define_ray_tracing_instance_collect_class(RayTracingInstanceCollectCustom)
        INNER_VISIBLE:
            struct InstanceInfo {
                InstanceAddressInfo address_info;
                CustomInstanceInfo custom_info;
            };
            struct InstanceCreateInfo {
                std::shared_ptr<Model> model;
                CustomInstanceInfo custom_info;
                glm::mat4 transform;
            };
            struct GroupInfo {
                uint32_t instance_start;
                uint32_t instance_count;
                std::vector<InstanceCreateInfo> instance_create_infos;
            };
            using update_custom_info_func = std::function<void(uint32_t, CustomInstanceInfo &)>;
        public:
            RayTracingInstanceCollectCustom &add_instance(uint32_t group, std::shared_ptr<Model> model, const CustomInstanceInfo &custom_info, const glm::mat4 &matrix = glm::mat4(1.0f));
            RayTracingInstanceCollectCustom &update_custom_infos(uint32_t group, update_custom_info_func update_custom_info);
        INNER_VISIBLE:
            void prepare_instance_infos() override;
            std::vector<InstanceInfo> instance_infos;
            std::map<uint32_t, GroupInfo> group_infos;
        };
        using type = RayTracingInstanceCollectCustom;
    };

    template <class CustomInstanceInfo>
    struct RayTracingInstanceCollectTrait<CustomInstanceInfo, true> : public RayTracingInstanceCollectType {
        class RayTracingInstanceCollectVoid final : public RayTracingInstanceCollectBase {
            no_copy_move_construction(RayTracingInstanceCollectVoid)
            define_ray_tracing_instance_collect_class(RayTracingInstanceCollectVoid)
        INNER_VISIBLE:
            using InstanceInfo = InstanceAddressInfo;
            struct InstanceCreateInfo {
                std::shared_ptr<Model> model;
                glm::mat4 transform;
            };
            struct GroupInfo {
                uint32_t instance_start;
                uint32_t instance_count;
                std::vector<InstanceCreateInfo> instance_create_infos;
            };
        public:
            RayTracingInstanceCollectVoid &add_instance(uint32_t group, std::shared_ptr<Model> model, const glm::mat4 &matrix = glm::mat4(1.0f));
        INNER_VISIBLE:
            void prepare_instance_infos() override;
            std::vector<InstanceInfo> instance_infos;
            std::map<uint32_t, GroupInfo> group_infos;
        };
        using type = RayTracingInstanceCollectVoid;
    };

    #undef define_ray_tracing_instance_class

    template <class CustomInstanceInfo>
    typename RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type &RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type::add_instance(uint32_t group, std::shared_ptr<Model> model, const CustomInstanceInfo &custom_info, const glm::mat4 &matrix) {
        if (!check_model(*model)) {
            return *this;
        }
        if (group_infos.find(group) == group_infos.end()) {
            group_infos[group] = {};
        }
        group_infos.at(group).instance_create_infos.push_back({ model, std::move(custom_info), matrix });
        return *this;
    };
    
    template <class CustomInstanceInfo>
    typename RayTracingInstanceCollectTrait<CustomInstanceInfo, true>::type &RayTracingInstanceCollectTrait<CustomInstanceInfo, true>::type::add_instance(uint32_t group, std::shared_ptr<Model> model, const glm::mat4 &matrix) {
        if (!check_model(*model)) {
            return *this;
        }
        if (group_infos.find(group) == group_infos.end()) {
            group_infos[group] = {};
        }
        group_infos.at(group).instance_create_infos.push_back({ model, matrix });
        return *this;
    };

    template <class CustomInstanceInfo>
    void RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type::prepare_instance_infos() {
        for (auto &[group_id, group_info] : group_infos) {
            group_info.instance_start = instance_infos.size();
            group_info.instance_count = group_info.instance_create_infos.size();
            for (auto &instance_create_info : group_info.instance_create_infos) {
                auto [blas_address, vertex_buffer_address, index_buffer_address] = get_model_addresses(*instance_create_info.model);
                acceleration_struction_instance_infos.emplace_back()
                    .setInstanceCustomIndex(instance_infos.size())
                    .setInstanceShaderBindingTableRecordOffset(0)
                    .setAccelerationStructureReference(blas_address)
                    .setMask(0xff)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                    .setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(instance_create_info.transform));
                instance_infos.push_back({
                    { vertex_buffer_address, index_buffer_address },
                    std::move(instance_create_info.custom_info),
                });
            }
        }
        instance_infos_buffer = std::make_shared<TwoStageBuffer>(instance_infos.size() * sizeof(InstanceInfo), vk::BufferUsageFlagBits::eStorageBuffer);
        instance_infos_buffer->upload_data_from_vector(instance_infos);
    };

    template <class CustomInstanceInfo>
    void RayTracingInstanceCollectTrait<CustomInstanceInfo, true>::type::prepare_instance_infos() {
        for (auto &[group_id, group_info] : group_infos) {
            group_info.instance_start = instance_infos.size();
            group_info.instance_count = group_info.instance_create_infos.size();
            for (auto &instance_create_info : group_info.instance_create_infos) {
                auto [blas_address, vertex_buffer_address, index_buffer_address] = get_model_addresses(*instance_create_info.model);
                acceleration_struction_instance_infos.emplace_back()
                    .setInstanceCustomIndex(instance_infos.size())
                    .setInstanceShaderBindingTableRecordOffset(0)
                    .setAccelerationStructureReference(blas_address)
                    .setMask(0xff)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                    .setTransform(transform<vk::TransformMatrixKHR, const glm::mat4 &>(instance_create_info.transform));
                instance_infos.push_back({
                    vertex_buffer_address,
                    index_buffer_address,
                });
            }
        }
        instance_infos_buffer = std::make_shared<TwoStageBuffer>(instance_infos.size() * sizeof(InstanceInfo), vk::BufferUsageFlagBits::eStorageBuffer);
        instance_infos_buffer->upload_data_from_vector(instance_infos);
    };

    template <class CustomInstanceInfo>
    typename RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type &RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type::update_custom_infos(uint32_t group, update_custom_info_func update_custom_info){
        auto &group_info = group_infos.at(group);
        uint32_t batch_begin = group_info.instance_start;
        uint32_t end = batch_begin + group_info.instance_count;
        auto update_custom_info_batch = [&](uint32_t batch_begin, uint32_t batch_end) {
            auto current_instance_info = (InstanceInfo *)instance_infos_buffer->map();
            for (uint32_t i = 0; i < batch_begin; i ++) {
                current_instance_info ++;
            }
            while (batch_begin < batch_end) {
                update_custom_info(batch_begin - group_info.instance_start, current_instance_info->custom_info);
                current_instance_info ++;
                batch_begin += 1;
            }
        };
        std::vector<std::thread> thread_pool;
        while (batch_begin < end) {
            uint32_t batch_end = std::min(batch_begin + 500, end);
            thread_pool.emplace_back([&]() {
                update_custom_info_batch(batch_begin, batch_end);
            });
            // thread_pool.back().join();
            batch_begin = batch_end;
        }
        for (auto &thread : thread_pool) {
            thread.join();
        }
        instance_infos_buffer->flush();
        return *this;
    }

    using RayTracingInstanceCollect = typename RayTracingInstanceCollectTrait<void, true>::type;
    template <class CustomInstanceInfo>
    using RayTracingInstanceCollectWithCustomInstanceInfo = typename RayTracingInstanceCollectTrait<CustomInstanceInfo, false>::type;
}
