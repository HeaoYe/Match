#include <Match/vulkan/descriptor_resource/descriptor_set.hpp>
#include <Match/vulkan/renderer.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    DescriptorSet::DescriptorSet(std::weak_ptr<Renderer> renderer) : allocated(false), renderer(renderer) {
    }

    DescriptorSet &DescriptorSet::add_descriptors(const std::vector<DescriptorInfo> &descriptor_infos) {
        for (auto &descriptor_info : descriptor_infos) {
            layout_bindings.push_back({
                descriptor_info.binding,
                transform<vk::DescriptorType>(descriptor_info.type),
                descriptor_info.count,
                transform<vk::ShaderStageFlags>(descriptor_info.stages),
                descriptor_info.immutable_samplers
            });
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::allocate() {
        if (allocated) {
            this->free();
        }
        allocated = true;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {};
        descriptor_set_layout_create_info.setBindings(layout_bindings);
        descriptor_layout = manager->device->device.createDescriptorSetLayout(descriptor_set_layout_create_info);

        descriptor_sets = manager->descriptor_pool->allocate_descriptor_sets(descriptor_layout);

        return *this;
    }

    DescriptorSet &DescriptorSet::free() {
        if (!allocated) {
            return *this;
        }
        allocated = false;

        manager->descriptor_pool->free_descriptor_sets(descriptor_sets);
        descriptor_sets.clear();
        manager->device->device.destroyDescriptorSetLayout(descriptor_layout);
        
        return *this;
    }

    vk::DescriptorSetLayoutBinding &DescriptorSet::get_layout_binding(uint32_t binding) {
        for (auto &layout_binding : layout_bindings) {
            if (layout_binding.binding == binding) {
                return layout_binding;
            }
        }
        MCH_ERROR("Failed find layout bining {}", binding);
        return *layout_bindings.end();
    }

    DescriptorSet::~DescriptorSet() {
        if (callback_id.has_value()) {
            if (!renderer.expired()) {
                renderer.lock()->remove_resource_recreate_callback(callback_id.value());
            }
            callback_id.reset();
        }
        this->free();
        layout_bindings.clear();
    }

    DescriptorSet &DescriptorSet::bind_uniforms(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>> &uniform_buffers) {
        auto layout_binding = get_layout_binding(binding);
        if (layout_binding.descriptorType != vk::DescriptorType::eUniformBuffer) {
            MCH_ERROR("Binding {} is not a uniform descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == uniform_buffers.size());
        for (uint32_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::DescriptorBufferInfo> buffer_infos(layout_binding.descriptorCount);
            for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                buffer_infos[i].setBuffer(uniform_buffers[i]->get_buffer(in_flight))
                    .setOffset(0)
                    .setRange(uniform_buffers[i]->size);
            }
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setBufferInfo(buffer_infos);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::bind_uniform(uint32_t binding, std::shared_ptr<UniformBuffer> uniform_buffer) {
        return bind_uniforms(binding, { uniform_buffer });
    }

    DescriptorSet &DescriptorSet::bind_textures(uint32_t binding, const std::vector<std::pair<std::shared_ptr<Texture>, std::shared_ptr<Sampler>>> &textures_samplers) {
        auto layout_binding = get_layout_binding(binding);
        if (layout_binding.descriptorType != vk::DescriptorType::eCombinedImageSampler) {
            MCH_ERROR("Binding {} is not a texture descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == textures_samplers.size());
        for (uint32_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::DescriptorImageInfo> image_infos(layout_binding.descriptorCount);
            for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                image_infos[i].setImageLayout(textures_samplers[i].first->get_image_layout())
                    .setImageView(textures_samplers[i].first->get_image_view())
                    .setSampler(textures_samplers[i].second->sampler);
            }
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setImageInfo(image_infos);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::bind_texture(uint32_t binding, std::shared_ptr<Texture> texture, std::shared_ptr<Sampler> sampler) {
        return bind_textures(binding, { { texture, sampler } });
    }

    DescriptorSet &DescriptorSet::bind_input_attachments(uint32_t binding, const std::vector<std::pair<std::string, std::shared_ptr<Sampler>>> &attachment_names_samplers) {
        auto locked_renderer = renderer.lock();
        if (!callback_id.has_value()) {
            callback_id = locked_renderer->register_resource_recreate_callback([this]() {
                update_input_attachments();
            });
        }
        auto layout_binding = get_layout_binding(binding);
        if ((layout_binding.descriptorType != vk::DescriptorType::eInputAttachment) && (layout_binding.descriptorType != vk::DescriptorType::eCombinedImageSampler)) {
            MCH_ERROR("Binding {} is not a InputAttachment descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == attachment_names_samplers.size());
        input_attachments_temp[binding] = attachment_names_samplers;
        for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::DescriptorImageInfo> image_infos(layout_binding.descriptorCount);
            for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                auto attachment_idx = locked_renderer->render_pass_builder->get_attachment_index(attachment_names_samplers[i].first, true);
                auto &attachment = locked_renderer->framebuffer_set->attachments[attachment_idx];
                image_infos[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setImageView(attachment.image_view)
                    .setSampler(attachment_names_samplers[i].second->sampler);
            }
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setImageInfo(image_infos);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::bind_input_attachment(uint32_t binding, const std::string &attachment_name, std::shared_ptr<Sampler> sampler) {
        return bind_input_attachments(binding, { {attachment_name, sampler } });
    }

    void DescriptorSet::update_input_attachments() {
        for (auto [binding, args] : input_attachments_temp) {
            bind_input_attachments(binding, args);
        }
    };
    DescriptorSet &DescriptorSet::bind_storage_buffers(uint32_t binding, const std::vector<std::shared_ptr<StorageBuffer>> &storage_buffers) {
        auto layout_binding = get_layout_binding(binding);
        if (layout_binding.descriptorType != vk::DescriptorType::eStorageBuffer) {
            MCH_ERROR("Binding {} is not a storage buffer descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == storage_buffers.size());
        for (uint32_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::DescriptorBufferInfo> buffer_infos(layout_binding.descriptorCount);
            for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                buffer_infos[i].setBuffer(storage_buffers[i]->get_buffer(in_flight))
                    .setOffset(0)
                    .setRange(storage_buffers[i]->get_size());
            }
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setBufferInfo(buffer_infos);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::bind_storage_buffer(uint32_t binding, std::shared_ptr<StorageBuffer> storage_buffer) {
        return bind_storage_buffers(binding, { storage_buffer });
    }
 
    DescriptorSet &DescriptorSet::bind_storage_images(uint32_t binding, const std::vector<std::shared_ptr<StorageImage>> &storage_images) {
        auto layout_binding = get_layout_binding(binding);
        if (layout_binding.descriptorType != vk::DescriptorType::eStorageImage) {
            MCH_ERROR("Binding {} is not a storage image descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == storage_images.size());
        for (uint32_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::DescriptorImageInfo> image_infos(layout_binding.descriptorCount);
            for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                image_infos[i].setImageLayout(vk::ImageLayout::eGeneral)
                    .setImageView(storage_images[i]->image_view);
            }
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setImageInfo(image_infos);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }
 
    DescriptorSet &DescriptorSet::bind_storage_image(uint32_t binding, std::shared_ptr<StorageImage> storage_image) {
        return bind_storage_images(binding, { storage_image });
    }
 
    DescriptorSet &DescriptorSet::bind_ray_tracing_instance_collects(uint32_t binding, const std::vector<std::shared_ptr<RayTracingInstanceCollect>> &instance_collects) {
        auto layout_binding = get_layout_binding(binding);
        if (layout_binding.descriptorType != vk::DescriptorType::eAccelerationStructureKHR) {
            MCH_ERROR("Binding {} is not a acceleration struction descriptor", binding)
            return *this;
        }
        assert(layout_binding.descriptorCount == instance_collects.size());
        for (uint32_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
            std::vector<vk::AccelerationStructureKHR> acceleration_structures;
            acceleration_structures.reserve(instance_collects.size());
            for (auto &instance_collect : instance_collects) {
                acceleration_structures.push_back(instance_collect->instance_collect);
            }
            vk::WriteDescriptorSetAccelerationStructureKHR acceleration_structure_write {};
            acceleration_structure_write.setAccelerationStructures(acceleration_structures);
            vk::WriteDescriptorSet descriptor_write {};
            descriptor_write.setDstSet(descriptor_sets[in_flight])
                .setDstBinding(layout_binding.binding)
                .setDstArrayElement(0)
                .setDescriptorType(layout_binding.descriptorType)
                .setDescriptorCount(acceleration_structure_write.accelerationStructureCount)
                .setPNext(&acceleration_structure_write);
            manager->device->device.updateDescriptorSets({ descriptor_write }, {});
        }
        return *this;
    }

    DescriptorSet &DescriptorSet::bind_ray_tracing_instance_collect(uint32_t binding, std::shared_ptr<RayTracingInstanceCollect> instance_collect) {
        return bind_ray_tracing_instance_collects(binding, { instance_collect });
    }
}
