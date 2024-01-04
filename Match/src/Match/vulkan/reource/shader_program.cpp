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

    ShaderProgram &ShaderProgram::attach_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set) {
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
    
    ShaderProgram &ShaderProgram::attach_descriptor_set(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t set_index) {
        if (set_index + 1 > descriptor_sets.size()) {
            descriptor_sets.resize(set_index + 1);
        }
        descriptor_sets[set_index] = descriptor_set;
        return *this;
    }

    ShaderProgram &ShaderProgram::compile(const ShaderProgramCompileOptions &options) {
        std::vector<vk::PipelineShaderStageCreateInfo> stages = {
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, vertex_shader->module.value(), vertex_shader_entry),
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, fragment_shader->module.value(), fragment_shader_entry),
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
        multisample_state.rasterizationSamples = transform<vk::SampleCountFlagBits>(runtime_setting->multisample_count);
        multisample_state.sampleShadingEnable = VK_TRUE;
        multisample_state.minSampleShading = 0.2f;
        multisample_state.setRasterizationSamples(transform<vk::SampleCountFlagBits>(runtime_setting->multisample_count))
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

        std::vector<vk::PushConstantRange> constant_ranges;
        uint32_t current_offset = 0;
        constant_ranges.reserve(2);
        if (vertex_shader->constants_size != 0) {
            constant_offset_size_map.insert(std::make_pair(vk::ShaderStageFlagBits::eVertex, std::make_pair(current_offset, vertex_shader->constants_size)));
            constant_ranges.push_back({
                vk::ShaderStageFlagBits::eVertex,
                current_offset,
                vertex_shader->constants_size,
            });
            current_offset += vertex_shader->constants_size;
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

        std::vector<vk::DescriptorSetLayout> descriptor_layouts;
        descriptor_layouts.reserve(descriptor_sets.size());
        for (auto &descriptor_set : descriptor_sets) {
            if (!descriptor_set.has_value()) {
                MCH_ERROR("Miss descriptor set at index {}", descriptor_layouts.size());
            } else if (!descriptor_set.value()->is_allocated()) {
                MCH_DEBUG("Auto allocate descriptor set")
                descriptor_set.value()->allocate();
            }
            descriptor_layouts.push_back(descriptor_set.value()->descriptor_layout);
        }
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.setSetLayouts(descriptor_layouts)
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

        return *this;
    }

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
        manager->device->device.destroyPipeline(pipeline);
        manager->device->device.destroyPipelineLayout(layout);
        descriptor_sets.clear();
        vertex_attribute_set.reset();
        vertex_shader.reset();
        fragment_shader.reset();
    }
}
