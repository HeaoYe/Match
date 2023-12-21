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
        multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisample_state.sampleShadingEnable = VK_FALSE;

        VkPipelineDepthStencilStateCreateInfo depth_stencil_state { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depth_stencil_state.depthTestEnable = VK_FALSE;
        depth_stencil_state.depthWriteEnable = VK_FALSE;
        depth_stencil_state.depthCompareOp = VK_COMPARE_OP_NEVER;
        depth_stencil_state.depthBoundsTestEnable = VK_FALSE;
        depth_stencil_state.stencilTestEnable = VK_FALSE;

        auto locked_renderer = renderer.lock();
        uint32_t subpass_idx = locked_renderer->render_pass_builder->subpasses_map.at(subpass_name);
        auto &subpass = locked_renderer->render_pass_builder->subpass_builders[subpass_idx];
        bind_point = locked_renderer->render_pass_builder->subpass_builders[subpass_idx].bind_point;

        VkPipelineColorBlendStateCreateInfo color_blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.attachmentCount = subpass.output_attachment_blend_states.size();
        color_blend_state.pAttachments = subpass.output_attachment_blend_states.data();

        VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.dynamicStateCount = options.dynamic_states.size();
        dynamic_state.pDynamicStates = options.dynamic_states.data();

        std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
        layout_bindings.reserve(vertex_shader->layout_bindings.size() + fragment_shader->layout_bindings.size());
        for (const auto &layout_binding : vertex_shader->layout_bindings) {
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        }
        for (const auto &layout_binding : fragment_shader->layout_bindings) {
            layout_bindings.push_back(layout_binding);
            layout_bindings.back().stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        descriptor_set_layout_create_info.bindingCount = layout_bindings.size();
        descriptor_set_layout_create_info.pBindings = layout_bindings.data();
        vkCreateDescriptorSetLayout(manager->device->device, &descriptor_set_layout_create_info, manager->allocator, &descriptor_set_layout);

        VkPipelineLayoutCreateInfo pipeline_layout_create_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_create_info.setLayoutCount = 1;
        pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout;
        pipeline_layout_create_info.pushConstantRangeCount = 0;
        pipeline_layout_create_info.pPushConstantRanges = nullptr;
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
            for (const auto &layout_binding : shader->layout_bindings) {
                if (layout_binding.binding == binding) {
                    if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                        MCH_ERROR("Binding {} is not a uniform descriptor", binding)
                        return;
                    }
                    for (size_t i = 0; i < setting.max_in_flight_frame; i++) {
                        std::vector<VkDescriptorBufferInfo> buffer_infos(layout_binding.descriptorCount);
                        for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                            buffer_infos[i].buffer = uniform_buffers[i]->buffers[i].buffer;
                            buffer_infos[i].offset = 0;
                            buffer_infos[i].range = uniform_buffers[i]->size;
                        }
                        VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                        descriptor_write.dstSet = descriptor_sets[i];
                        descriptor_write.dstBinding = layout_binding.binding;
                        descriptor_write.dstArrayElement = 0;
                        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                        descriptor_write.descriptorCount = layout_binding.descriptorCount;
                        descriptor_write.pBufferInfo = buffer_infos.data();
                        vkUpdateDescriptorSets(manager->device->device, 1, &descriptor_write, 0, nullptr);
                    }
                    return;
                }
            }
        }
    }

    void ShaderProgram::bind_textures(binding binding, const std::vector<std::shared_ptr<Texture>> &textures, const std::vector<std::shared_ptr<Sampler>> &samplers) {
        for (const auto &shader : { vertex_shader, fragment_shader }) {
            for (const auto &layout_binding : shader->layout_bindings) {
                if (layout_binding.binding == binding) {
                    if (layout_binding.descriptorType != VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                        MCH_ERROR("Binding {} is not a texture descriptor", binding)
                        return;
                    }
                    for (size_t i = 0; i < setting.max_in_flight_frame; i++) {
                        std::vector<VkDescriptorImageInfo> image_infos(layout_binding.descriptorCount);
                        for (uint32_t i = 0; i < layout_binding.descriptorCount; i ++) {
                            image_infos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                            image_infos[i].imageView = textures[i]->image_view;
                            image_infos[i].sampler = samplers[i]->sampler;
                        }
                        VkWriteDescriptorSet descriptor_write { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
                        descriptor_write.dstSet = descriptor_sets[i];
                        descriptor_write.dstBinding = layout_binding.binding;
                        descriptor_write.dstArrayElement = 0;
                        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                        descriptor_write.descriptorCount = layout_binding.descriptorCount;
                        descriptor_write.pImageInfo = image_infos.data();
                        vkUpdateDescriptorSets(manager->device->device, 1, &descriptor_write, 0, nullptr);
                    }
                    return;
                }
            }
        }
    }

    ShaderProgram::~ShaderProgram() {
        descriptor_sets.clear();
        vkDestroyPipeline(manager->device->device, pipeline, manager->allocator);
        vkDestroyDescriptorSetLayout(manager->device->device, descriptor_set_layout, manager->allocator);
        vkDestroyPipelineLayout(manager->device->device, layout, manager->allocator);
        vertex_attribute_set.reset();
        vertex_shader.reset();
        fragment_shader.reset();
    }
}
