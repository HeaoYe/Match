#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/core/setting.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    static vk::PipelineShaderStageCreateInfo create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits stage, vk::ShaderModule module, const std::string &entry) {
        vk::PipelineShaderStageCreateInfo create_info {};
        create_info.pSpecializationInfo = nullptr;
        create_info.stage = stage;
        create_info.module = module;
        create_info.pName = entry.c_str();
        create_info.setPSpecializationInfo(nullptr)
            .setStage(stage)
            .setModule(module)
            .setPName(entry.c_str());
        return create_info;
    }
    
    ShaderProgram::ShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name) : subpass_name(subpass_name), renderer(renderer) {}

    ShaderProgram &ShaderProgram::bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set) {
        vertex_attribute_set.reset();
        vertex_attribute_set = std::move(attribute_set);
        return *this;
    }

    ShaderProgram &ShaderProgram::attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        vertex_shader = std::dynamic_pointer_cast<Shader>(shader);
        vertex_shader_entry = entry;
        return *this;
    }

    ShaderProgram &ShaderProgram::attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        fragment_shader = std::dynamic_pointer_cast<Shader>(shader);
        fragment_shader_entry = entry;
        return *this;
    }

    ShaderProgram &ShaderProgram::compile(const ShaderProgramCompileOptions &options) {
        std::vector<vk::PipelineShaderStageCreateInfo> stages = {
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, vertex_shader->module, vertex_shader_entry),
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, fragment_shader->module, fragment_shader_entry),
        };

        vk::PipelineVertexInputStateCreateInfo vertex_input_state {};
        if (vertex_attribute_set.get() != nullptr) {
            vertex_input_state.setVertexBindingDescriptions(vertex_attribute_set->bindings)
                .setVertexAttributeDescriptions(vertex_attribute_set->attributes);
        }

        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state {};
        input_assembly_state.setTopology(transform<vk::PrimitiveTopology>(options.topology))
            .setPrimitiveRestartEnable(VK_FALSE);

        auto &size = runtime_setting->get_window_size();
        vk::Viewport viewport {
            0.0f,
            static_cast<float>(size.height),
            static_cast<float>(size.width),
            -static_cast<float>(size.height),
            0.0f,
            1.0f,
        };
        vk::Rect2D scissor {
            { 0, 0 },
            { size.width, size.height },
        };
        vk::PipelineViewportStateCreateInfo viewport_state {};
        viewport_state.setViewports(viewport)
            .setScissors(scissor);

        vk::PipelineRasterizationStateCreateInfo rasterization_state {};
        rasterization_state.setRasterizerDiscardEnable(VK_FALSE)
            .setPolygonMode(transform<vk::PolygonMode>(options.polygon_mode))
            .setDepthBiasEnable(VK_FALSE)
            .setDepthBiasClamp(VK_FALSE)
            .setFrontFace(transform<vk::FrontFace>(options.front_face))
            .setCullMode(transform<vk::CullModeFlags>(options.cull_mode))
            .setLineWidth(1.0f);

        vk::PipelineMultisampleStateCreateInfo multisample_state {};
        multisample_state.rasterizationSamples = runtime_setting->multisample_count;
        multisample_state.sampleShadingEnable = VK_TRUE;
        multisample_state.minSampleShading = 0.2f;
        multisample_state.setRasterizationSamples(runtime_setting->multisample_count)
            .setSampleShadingEnable(VK_TRUE)
            .setMinSampleShading(0.2f);

        vk::PipelineDepthStencilStateCreateInfo depth_stencil_state {};
        depth_stencil_state.setDepthTestEnable(options.depth_test_enable)
            .setDepthWriteEnable(options.depth_write_enable)
            .setDepthCompareOp(options.depth_compere_op)
            .setDepthBoundsTestEnable(VK_FALSE)
            .setStencilTestEnable(VK_FALSE);

        auto locked_renderer = renderer.lock();
        uint32_t subpass_idx = locked_renderer->render_pass_builder->get_subpass_index(subpass_name);
        auto &subpass = *locked_renderer->render_pass_builder->subpass_builders[subpass_idx];
        bind_point = subpass.bind_point;

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.setLogicOpEnable(VK_FALSE)
            .setAttachments(subpass.output_attachment_blend_states);

        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        dynamic_state.setDynamicStates(options.dynamic_states);

        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        std::vector<vk::PushConstantRange> constant_ranges;
        uint32_t current_offset = 0;
        layout_bindings.reserve(vertex_shader->layout_bindings.size() + fragment_shader->layout_bindings.size());
        constant_ranges.reserve(2);
        for (const auto &layout_binding : vertex_shader->layout_bindings) {
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = vk::ShaderStageFlagBits::eVertex;
        }
        if (vertex_shader->constants_size != 0) {
            constant_offset_size_map.insert(std::make_pair(vk::ShaderStageFlagBits::eVertex, std::make_pair(current_offset, vertex_shader->constants_size)));
            constant_ranges.push_back({
                vk::ShaderStageFlagBits::eVertex,
                current_offset,
                vertex_shader->constants_size,
            });
            current_offset += vertex_shader->constants_size;
        }
        for (const auto &layout_binding : fragment_shader->layout_bindings) {
            bool exist = false;
            for (auto &exist_binding : layout_bindings) {
                if (exist_binding.binding == layout_binding.binding) {
                    assert(exist_binding.descriptorType == layout_binding.descriptorType);
                    exist_binding.stageFlags |= vk::ShaderStageFlagBits::eFragment;
                    exist = true;
                }
            }
            if (exist) {
                continue;
            }
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = vk::ShaderStageFlagBits::eFragment;
        }
        if (fragment_shader->constants_size != 0) {
            if (current_offset % fragment_shader->first_align.value() != 0) {
                current_offset += fragment_shader->first_align.value() - (current_offset % fragment_shader->first_align.value());
            }
            constant_offset_size_map.insert(std::make_pair(vk::ShaderStageFlagBits::eFragment, std::make_pair(current_offset, fragment_shader->constants_size)));
            constant_ranges.push_back({
                vk::ShaderStageFlagBits::eFragment,
                current_offset,
                fragment_shader->constants_size,
            });
            current_offset += fragment_shader->constants_size;
        }
        constants.resize(current_offset);
        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info {};
        descriptor_set_layout_create_info.setBindings(layout_bindings);
        descriptor_set_layout = manager->device->device.createDescriptorSetLayout(descriptor_set_layout_create_info);

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = constant_ranges.size();
        pipeline_layout_create_info.pPushConstantRanges = constant_ranges.data();
        pipeline_layout_create_info.setSetLayouts(descriptor_set_layout)
            .setPushConstantRanges(constant_ranges);
        layout = manager->device->device.createPipelineLayout(pipeline_layout_create_info);

        vk::GraphicsPipelineCreateInfo create_info {};
        create_info.setStages(stages)
            .setPVertexInputState(&vertex_input_state)
            .setPInputAssemblyState(&input_assembly_state)
            .setPTessellationState(nullptr)
            .setPViewportState(&viewport_state)
            .setPRasterizationState(&rasterization_state)
            .setPMultisampleState(&multisample_state)
            .setPDepthStencilState(&depth_stencil_state)
            .setPColorBlendState(&color_blend_state)
            .setPDynamicState(&dynamic_state)
            .setLayout(layout)
            .setRenderPass(locked_renderer->render_pass->render_pass)
            .setSubpass(subpass_idx)
            .setBasePipelineHandle(nullptr)
            .setBasePipelineIndex(0);
        locked_renderer.reset();

        pipeline = manager->device->device.createGraphicsPipelines(nullptr, { create_info }).value[0];

        descriptor_sets = manager->descriptor_pool->allocate_descriptor_set(descriptor_set_layout);
        return *this;
    }

    ShaderProgram &ShaderProgram::bind_uniforms(binding binding, const std::vector<std::shared_ptr<UniformBuffer>> &uniform_buffers) {
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            auto *layout_binding = shader->get_layout_binding(binding);
            if (layout_binding == nullptr) {
                continue;
            }
            if (layout_binding->descriptorType != vk::DescriptorType::eUniformBuffer) {
                MCH_ERROR("Binding {} is not a uniform descriptor", binding)
                return *this;
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<vk::DescriptorBufferInfo> buffer_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    buffer_infos[i].setBuffer(uniform_buffers[i]->get_buffer(in_flight).buffer)
                        .setOffset(0)
                        .setRange(uniform_buffers[i]->size);
                }
                vk::WriteDescriptorSet descriptor_write {};
                descriptor_write.setDstSet(descriptor_sets[in_flight])
                    .setDstBinding(layout_binding->binding)
                    .setDstArrayElement(0)
                    .setDescriptorType(layout_binding->descriptorType)
                    .setBufferInfo(buffer_infos);
                manager->device->device.updateDescriptorSets({ descriptor_write }, {});
            }
        }
        return *this;
    }

    ShaderProgram &ShaderProgram::bind_textures(binding binding, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<Sampler>> &samplers) {
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            auto *layout_binding = shader->get_layout_binding(binding);
            if (layout_binding == nullptr) {
                continue;
            }
            if (layout_binding->descriptorType != vk::DescriptorType::eCombinedImageSampler) {
                MCH_ERROR("Binding {} is not a texture descriptor", binding)
                return *this;
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<vk::DescriptorImageInfo> image_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    image_infos[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                        .setImageView(textures[i]->get_image_view())
                        .setSampler(samplers[i]->sampler);
                }
                vk::WriteDescriptorSet descriptor_write {};
                descriptor_write.setDstSet(descriptor_sets[in_flight])
                    .setDstBinding(layout_binding->binding)
                    .setDstArrayElement(0)
                    .setDescriptorType(layout_binding->descriptorType)
                    .setImageInfo(image_infos);
                manager->device->device.updateDescriptorSets({ descriptor_write }, {});
            }
        }
        return *this;
    }
    
    ShaderProgram &ShaderProgram::bind_input_attachments(binding binding, const std::vector<std::string> &attachment_names, const std::vector<std::shared_ptr<Sampler>> &samplers) {
        auto locked_renderer = renderer.lock();
        if (!callback_id.has_value()) {
            callback_id = locked_renderer->register_resource_recreate_callback([this]() {
                update_input_attachments();
            });
        }
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            auto *layout_binding = shader->get_layout_binding(binding);
            if (layout_binding == nullptr) {
                continue;
            }
            if ((layout_binding->descriptorType != vk::DescriptorType::eInputAttachment) && (layout_binding->descriptorType != vk::DescriptorType::eCombinedImageSampler)) {
                MCH_ERROR("Binding {} is not a InputAttachment descriptor", binding)
                return *this;
            }
            auto pos = input_attachments_temp.find(binding);
            if (pos == input_attachments_temp.end()) {
                input_attachments_temp.insert(std::make_pair(binding, std::make_pair(attachment_names, samplers)));
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<vk::DescriptorImageInfo> image_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    auto attachment_idx = locked_renderer->render_pass_builder->get_attachment_index(attachment_names[i], true);
                    auto &attachment = locked_renderer->framebuffer_set->attachments[attachment_idx];
                    image_infos[i].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                        .setImageView(attachment.image_view)
                        .setSampler(samplers[i]->sampler);
                }
                vk::WriteDescriptorSet descriptor_write {};
                descriptor_write.setDstSet(descriptor_sets[in_flight])
                    .setDstBinding(layout_binding->binding)
                    .setDstArrayElement(0)
                    .setDescriptorType(layout_binding->descriptorType)
                    .setImageInfo(image_infos);
                manager->device->device.updateDescriptorSets({ descriptor_write }, {});
            }
        }
        return *this;
    }

    void ShaderProgram::update_input_attachments() {
        for (auto [binding, info] : input_attachments_temp) {
            bind_input_attachments(binding, info.first, info.second);
        }
    };

    ShaderProgram &ShaderProgram::push_constants(const std::string &name, BasicConstantValue basic_value) {
        push_constants(name, &basic_value);
        return *this;
    }

    ShaderProgram &ShaderProgram::push_constants(const std::string &name, void *data) {
        auto [offset, size] = find_offset_size_by_name(*fragment_shader, vk::ShaderStageFlagBits::eFragment, name);
        if (offset == -1u) {
            auto [offset_, size_] = find_offset_size_by_name(*vertex_shader, vk::ShaderStageFlagBits::eVertex, name);
            if (offset_ == -1u) {
                return *this;
            }
            offset = offset_;
            size = size_;
        }
        memcpy(constants.data() + offset, data, size);
        return *this;
    }

    std::pair<uint32_t, uint32_t> ShaderProgram::find_offset_size_by_name(Shader &shader, vk::ShaderStageFlags stage, const std::string &name) {
        if (shader.constant_offset_map.find(name) != shader.constant_offset_map.end()) {
            return std::make_pair(constant_offset_size_map.at(stage).first + shader.constant_offset_map.at(name), shader.constant_size_map.at(name));
        }
        return std::make_pair(-1u, -1u);
    }

    ShaderProgram::~ShaderProgram() {
        if (callback_id.has_value()) {
            renderer.lock()->remove_resource_recreate_callback(callback_id.value());
            callback_id.reset();
        }
        for (auto &descriptor_set : descriptor_sets) {
            manager->descriptor_pool->free_descriptor_set(descriptor_set);
        }
        descriptor_sets.clear();
        manager->device->device.destroyPipeline(pipeline);
        manager->device->device.destroyDescriptorSetLayout(descriptor_set_layout);
        manager->device->device.destroyPipelineLayout(layout);
        vertex_attribute_set.reset();
        vertex_shader.reset();
        fragment_shader.reset();
    }
}
