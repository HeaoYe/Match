#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/core/setting.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    static VkPipelineShaderStageCreateInfo create_pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule module, const std::string &entry) {
        VkPipelineShaderStageCreateInfo create_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        create_info.pSpecializationInfo = nullptr;
        create_info.stage = stage;
        create_info.module = module;
        create_info.pName = entry.c_str();
        return create_info;
    }
    
    ShaderProgram::ShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name) : subpass_name(subpass_name), renderer(renderer) {}

    void ShaderProgram::bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set) {
        vertex_attribute_set.reset();
        vertex_attribute_set = std::move(attribute_set);
    }

    void ShaderProgram::attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        vertex_shader = std::dynamic_pointer_cast<Shader>(shader);
        vertex_shader_entry = entry;
    }

    void ShaderProgram::attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        fragment_shader = std::dynamic_pointer_cast<Shader>(shader);
        fragment_shader_entry = entry;
    }

    void ShaderProgram::compile(const ShaderProgramCompileOptions &options) {
        std::vector<VkPipelineShaderStageCreateInfo> stages = {
            create_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader->module, vertex_shader_entry),
            create_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader->module, fragment_shader_entry),
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        if (vertex_attribute_set.get() != nullptr) {
            vertex_input_state.vertexBindingDescriptionCount = vertex_attribute_set->bindings.size();
            vertex_input_state.pVertexBindingDescriptions = vertex_attribute_set->bindings.data();
            vertex_input_state.vertexAttributeDescriptionCount = vertex_attribute_set->attributes.size();
            vertex_input_state.pVertexAttributeDescriptions = vertex_attribute_set->attributes.data();
        } else {
            vertex_input_state.vertexBindingDescriptionCount = 0;
            vertex_input_state.pVertexBindingDescriptions = nullptr;
            vertex_input_state.vertexAttributeDescriptionCount = 0;
            vertex_input_state.pVertexAttributeDescriptions = nullptr;
        }

        VkPipelineInputAssemblyStateCreateInfo input_assembly_state { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        input_assembly_state.topology = transform<VkPrimitiveTopology>(options.topology);
        input_assembly_state.primitiveRestartEnable = VK_FALSE;

        auto &size = runtime_setting->get_window_size();
        VkViewport viewport {
            .x = 0.0f,
            .y = static_cast<float>(size.height),
            .width = static_cast<float>(size.width),
            .height = -static_cast<float>(size.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        VkRect2D scissor {
            .offset = { .x = 0, .y = 0 },
            .extent = { size.width, size.height },
        };
        VkPipelineViewportStateCreateInfo viewport_state { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewport_state.pViewports = &viewport;
        viewport_state.viewportCount = 1;
        viewport_state.pScissors = &scissor;
        viewport_state.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterization_state { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterization_state.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state.polygonMode = transform<VkPolygonMode>(options.polygon_mode);
        rasterization_state.depthBiasEnable = VK_FALSE;
        rasterization_state.depthClampEnable = VK_FALSE;
        rasterization_state.frontFace = transform<VkFrontFace>(options.front_face);
        rasterization_state.cullMode = transform<VkCullModeFlags>(options.cull_mode);
        rasterization_state.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample_state { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisample_state.rasterizationSamples = runtime_setting->multisample_count;
        multisample_state.sampleShadingEnable = VK_TRUE;
        multisample_state.minSampleShading = 0.2f;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depth_stencil_state.depthTestEnable = options.depth_test_enable;
        depth_stencil_state.depthWriteEnable = options.depth_write_enable;
        depth_stencil_state.depthCompareOp = options.depth_compere_op;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;

        auto locked_renderer = renderer.lock();
        uint32_t subpass_idx = locked_renderer->render_pass_builder->get_subpass_index(subpass_name);
        auto &subpass = *locked_renderer->render_pass_builder->subpass_builders[subpass_idx];
        bind_point = subpass.bind_point;

        VkPipelineColorBlendStateCreateInfo color_blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.attachmentCount = subpass.output_attachment_blend_states.size();
        color_blend_state.pAttachments = subpass.output_attachment_blend_states.data();

        VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.dynamicStateCount = options.dynamic_states.size();
        dynamic_state.pDynamicStates = options.dynamic_states.data();

        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        std::vector<VkPushConstantRange> constant_ranges;
        uint32_t current_offset = 0;
        layout_bindings.reserve(vertex_shader->layout_bindings.size() + fragment_shader->layout_bindings.size());
        constant_ranges.reserve(2);
        for (const auto &layout_binding : vertex_shader->layout_bindings) {
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (vertex_shader->constants_size != 0) {
            constant_offset_size_map.insert(std::make_pair(VK_SHADER_STAGE_VERTEX_BIT, std::make_pair(current_offset, vertex_shader->constants_size)));
            constant_ranges.push_back({
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = current_offset,
                .size = vertex_shader->constants_size,
            });
            current_offset += vertex_shader->constants_size;
        }
        for (const auto &layout_binding : fragment_shader->layout_bindings) {
            bool exist = false;
            for (auto &exist_binding : layout_bindings) {
                if (exist_binding.binding == layout_binding.binding) {
                    assert(exist_binding.descriptorType == layout_binding.descriptorType);
                    exist_binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
                    exist = true;
                }
            }
            if (exist) {
                continue;
            }
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (fragment_shader->constants_size != 0) {
            if (current_offset % fragment_shader->first_align.value() != 0) {
                current_offset += fragment_shader->first_align.value() - (current_offset % fragment_shader->first_align.value());
            }
            constant_offset_size_map.insert(std::make_pair(VK_SHADER_STAGE_FRAGMENT_BIT, std::make_pair(current_offset, fragment_shader->constants_size)));
            constant_ranges.push_back({
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = current_offset,
                .size = fragment_shader->constants_size,
            });
            current_offset += fragment_shader->constants_size;
        }
        constants.resize(current_offset);
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        descriptor_set_layout_create_info.bindingCount = layout_bindings.size();
        descriptor_set_layout_create_info.pBindings = layout_bindings.data();
        vkCreateDescriptorSetLayout(manager->device->device, &descriptor_set_layout_create_info, manager->allocator, &descriptor_set_layout);

        VkPipelineLayoutCreateInfo pipeline_layout_create_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = constant_ranges.size();
        pipeline_layout_create_info.pPushConstantRanges = constant_ranges.data();
        vk_check(vkCreatePipelineLayout(manager->device->device, &pipeline_layout_create_info, manager->allocator, &layout));

        VkGraphicsPipelineCreateInfo create_info { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        create_info.stageCount = stages.size();
        create_info.pStages = stages.data();
        create_info.pVertexInputState = &vertex_input_state;
        create_info.pInputAssemblyState = &input_assembly_state;
        create_info.pTessellationState = nullptr;
        create_info.pViewportState = &viewport_state;
        create_info.pRasterizationState = &rasterization_state;
        create_info.pMultisampleState = &multisample_state;
        create_info.pDepthStencilState = &depth_stencil_state;
        create_info.pColorBlendState = &color_blend_state;
        create_info.pDynamicState = &dynamic_state;
        create_info.layout = layout;
        create_info.renderPass = locked_renderer->render_pass->render_pass;
        create_info.subpass = subpass_idx;
        create_info.basePipelineHandle = nullptr;
        create_info.basePipelineIndex = 0;
        locked_renderer.reset();

        vk_check(vkCreateGraphicsPipelines(manager->device->device, nullptr, 1, &create_info, manager->allocator, &pipeline));

        descriptor_sets = manager->descriptor_pool->allocate_descriptor_set(descriptor_set_layout);
    }

    void ShaderProgram::bind_uniforms(binding binding, const std::vector<std::shared_ptr<UniformBuffer>> &uniform_buffers) {
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            auto *layout_binding = shader->get_layout_binding(binding);
            if (layout_binding == nullptr) {
                continue;
            }
            if (layout_binding->descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                MCH_ERROR("Binding {} is not a uniform descriptor", binding)
                return;
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<VkDescriptorBufferInfo> buffer_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    buffer_infos[i].buffer = uniform_buffers[i]->buffers[i].buffer;
                    buffer_infos[i].offset = 0;
                    buffer_infos[i].range = uniform_buffers[i]->size;
                }
                VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                descriptor_write.dstSet = descriptor_sets[in_flight];
                descriptor_write.dstBinding = layout_binding->binding;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = layout_binding->descriptorType;
                descriptor_write.descriptorCount = layout_binding->descriptorCount;
                descriptor_write.pBufferInfo = buffer_infos.data();
                vkUpdateDescriptorSets(manager->device->device, 1, &descriptor_write, 0, nullptr);
            }
        }
    }

    void ShaderProgram::bind_textures(binding binding, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<Sampler>> &samplers) {
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            auto *layout_binding = shader->get_layout_binding(binding);
            if (layout_binding == nullptr) {
                continue;
            }
            if (layout_binding->descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                MCH_ERROR("Binding {} is not a texture descriptor", binding)
                return;
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<VkDescriptorImageInfo> image_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    image_infos[i].imageView = textures[i]->get_image_view();
                    image_infos[i].sampler = samplers[i]->sampler;
                }
                VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                descriptor_write.dstSet = descriptor_sets[in_flight];
                descriptor_write.dstBinding = layout_binding->binding;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = layout_binding->descriptorType;
                descriptor_write.descriptorCount = layout_binding->descriptorCount;
                descriptor_write.pImageInfo = image_infos.data();
                vkUpdateDescriptorSets(manager->device->device, 1, &descriptor_write, 0, nullptr);
            }
        }
    }
    
    void ShaderProgram::bind_input_attachments(binding binding, const std::vector<std::string> &attachment_names, const std::vector<std::shared_ptr<Sampler>> &samplers) {
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
            if ((layout_binding->descriptorType & (VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT | VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)) == 0) {
                MCH_ERROR("Binding {} is not a InputAttachment descriptor", binding)
                return;
            }
            auto pos = input_attachments_temp.find(binding);
            if (pos == input_attachments_temp.end()) {
                input_attachments_temp.insert(std::make_pair(binding, std::make_pair(attachment_names, samplers)));
            }
            for (size_t in_flight = 0; in_flight < setting.max_in_flight_frame; in_flight ++) {
                std::vector<VkDescriptorImageInfo> image_infos(layout_binding->descriptorCount);
                for (uint32_t i = 0; i < layout_binding->descriptorCount; i ++) {
                    image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                    auto attachment_idx = locked_renderer->render_pass_builder->get_attachment_index(attachment_names[i], true);
                    auto &attachment = locked_renderer->framebuffer_set->attachments[attachment_idx];
                    image_infos[i].imageView = attachment.image_view;
                    image_infos[i].sampler = samplers[i]->sampler;
                }
                VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                descriptor_write.dstSet = descriptor_sets[in_flight];
                descriptor_write.dstBinding = layout_binding->binding;
                descriptor_write.dstArrayElement = 0;
                descriptor_write.descriptorType = layout_binding->descriptorType;
                descriptor_write.descriptorCount = layout_binding->descriptorCount;
                descriptor_write.pImageInfo = image_infos.data();
                vkUpdateDescriptorSets(manager->device->device, 1, &descriptor_write, 0, nullptr);
            }
        }
    }

    void ShaderProgram::update_input_attachments() {
        for (auto [binding, info] : input_attachments_temp) {
            bind_input_attachments(binding, info.first, info.second);
        }
    };

    void ShaderProgram::push_constants(const std::string &name, BasicConstantValue basic_value) {
        push_constants(name, &basic_value);
    }

    void ShaderProgram::push_constants(const std::string &name, void *data) {
        auto [offset, size] = find_offset_size_by_name(*fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT, name);
        if (offset == -1u) {
            auto [offset_, size_] = find_offset_size_by_name(*vertex_shader, VK_SHADER_STAGE_VERTEX_BIT, name);
            if (offset_ == -1u) {
                return;
            }
            offset = offset_;
            size = size_;
        }
        memcpy(constants.data() + offset, data, size);
    }

    std::pair<uint32_t, uint32_t> ShaderProgram::find_offset_size_by_name(Shader &shader, VkShaderStageFlags stage, const std::string &name) {
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
        descriptor_sets.clear();
        vkDestroyPipeline(manager->device->device, pipeline, manager->allocator);
        vkDestroyDescriptorSetLayout(manager->device->device, descriptor_set_layout, manager->allocator);
        vkDestroyPipelineLayout(manager->device->device, layout, manager->allocator);
        vertex_attribute_set.reset();
        vertex_shader.reset();
        fragment_shader.reset();
    }
}
