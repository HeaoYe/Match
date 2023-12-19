#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/core/setting.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    VertexAttributeSet::VertexAttributeSet() {
        current_binding = 0;
        reset();
    }

    void VertexAttributeSet::reset() {
        current_location = 0;
        current_offset = 0;
    }

    VertexAttributeSet::location VertexAttributeSet::add_input_attribute(VertexType type) {
        attributes.push_back({
            .location = current_location,
            .binding = current_binding,
            .format = transform<VkFormat>(type),
            .offset = current_offset,
        });
        auto location = current_location, size = transform<uint32_t>(type);
        current_location += 1;
        if (size > 16) {
            current_location += 1;
        }
        current_offset += size;
        return location;
    }

    VertexAttributeSet::binding VertexAttributeSet::add_input_binding(InputRate rate) {
        bindings.push_back({
            .binding = current_binding,
            .stride = current_offset,
            .inputRate = transform<VkVertexInputRate>(rate),
        });
        current_binding += 1;
        reset();
        return current_binding - 1;
    }

    static VkPipelineShaderStageCreateInfo create_pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule module, const std::string &entry) {
        VkPipelineShaderStageCreateInfo create_info { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        create_info.pSpecializationInfo = nullptr;
        create_info.stage = stage;
        create_info.module = module;
        create_info.pName = entry.c_str();
        return create_info;
    }
    
    ShaderProgram::ShaderProgram(const std::string &subpass_name) : subpass_name(subpass_name) {}

    void ShaderProgram::bind_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute) {
        vertex_attribute.reset();
        vertex_attribute = std::move(attribute);
    }

    void ShaderProgram::attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        vertex_shader = std::dynamic_pointer_cast<Shader>(shader);
        vertex_shader_entry = entry;
    }

    void ShaderProgram::attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        fragment_shader = std::dynamic_pointer_cast<Shader>(shader);
        fragment_shader_entry = entry;
    }

    void ShaderProgram::compile(ShaderProgramCompileOptions options) {
        std::vector<VkPipelineShaderStageCreateInfo> stages = {
            create_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertex_shader->module, vertex_shader_entry),
            create_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragment_shader->module, fragment_shader_entry),
        };

        VkPipelineVertexInputStateCreateInfo vertex_input_state { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        if (vertex_attribute.get() != nullptr) {
            vertex_input_state.vertexBindingDescriptionCount = vertex_attribute->bindings.size();
            vertex_input_state.pVertexBindingDescriptions = vertex_attribute->bindings.data();
            vertex_input_state.vertexAttributeDescriptionCount = vertex_attribute->attributes.size();
            vertex_input_state.pVertexAttributeDescriptions = vertex_attribute->attributes.data();
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

        uint32_t subpass_idx = manager->render_pass_builder->subpasses_map.at(subpass_name);
        auto &subpass = manager->render_pass_builder->subpass_builders[subpass_idx];
        bind_point = manager->render_pass_builder->subpass_builders[subpass_idx].bind_point;

        VkPipelineColorBlendStateCreateInfo color_blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        color_blend_state.logicOpEnable = VK_FALSE;
        color_blend_state.attachmentCount = subpass.output_attachment_blend_states.size();
        color_blend_state.pAttachments = subpass.output_attachment_blend_states.data();

        std::vector<VkDynamicState> dynamic_states = {
            // VK_DYNAMIC_STATE_VIEWPORT,
            // VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamic_state.dynamicStateCount = dynamic_states.size();
        dynamic_state.pDynamicStates = dynamic_states.data();

        VkPipelineLayoutCreateInfo pipeline_layout_create_info { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipeline_layout_create_info.setLayoutCount = 0;
        pipeline_layout_create_info.pSetLayouts = nullptr;
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
        create_info.renderPass = manager->render_pass->render_pass;
        create_info.subpass = subpass_idx;
        create_info.basePipelineHandle = nullptr;
        create_info.basePipelineIndex = 0;

        vk_check(vkCreateGraphicsPipelines(manager->device->device, nullptr, 1, &create_info, manager->allocator, &pipeline));
    }

    ShaderProgram::~ShaderProgram() {
        vkDestroyPipeline(manager->device->device, pipeline, manager->allocator);
        vkDestroyPipelineLayout(manager->device->device, layout, manager->allocator);
    }
}
