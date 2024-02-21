#include <Match/vulkan/renderer.hpp>
#include <Match/vulkan/resource/shader_program.hpp>
#include <Match/vulkan/utils.hpp>
#include <Match/core/setting.hpp>
#include <Match/core/utils.hpp>
#include "../inner.hpp"

namespace Match {
    static vk::PipelineShaderStageCreateInfo create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits stage, vk::ShaderModule module, const std::string &entry) {
        vk::PipelineShaderStageCreateInfo create_info {};
        create_info.setPSpecializationInfo(nullptr)
            .setStage(stage)
            .setModule(module)
            .setPName(entry.c_str());
        return create_info;
    }

    ShaderProgram::~ShaderProgram() {
        manager->device->device.destroyPipeline(pipeline);
        manager->device->device.destroyPipelineLayout(layout);
        descriptor_sets.clear();
    }

    void ShaderProgram::compile_pipeline_layout() {
        std::vector<vk::DescriptorSetLayout> descriptor_layouts;
        descriptor_layouts.reserve(descriptor_sets.size());
        for (auto &descriptor_set : descriptor_sets) {
            if (!descriptor_set.has_value()) {
                MCH_ERROR("Miss descriptor set at index {}", descriptor_layouts.size());
            } else if (!descriptor_set.value()->allocated) {
                MCH_DEBUG("Auto allocate descriptor set")
                descriptor_set.value()->allocate();
            }
            descriptor_layouts.push_back(descriptor_set.value()->descriptor_layout);
        }

        vk::PipelineLayoutCreateInfo pipeline_layout_create_info {};
        pipeline_layout_create_info.setSetLayouts(descriptor_layouts);
        if (push_constants.has_value()) {
            pipeline_layout_create_info.setPushConstantRanges(push_constants.value()->range);
        }
        layout = manager->device->device.createPipelineLayout(pipeline_layout_create_info);
    }

    GraphicsShaderProgram::GraphicsShaderProgram(std::weak_ptr<Renderer> renderer, const std::string &subpass_name) : renderer(renderer), subpass_name(subpass_name) {}

    GraphicsShaderProgram &GraphicsShaderProgram::attach_vertex_attribute_set(std::shared_ptr<VertexAttributeSet> attribute_set) {
        vertex_attribute_set.reset();
        vertex_attribute_set = std::move(attribute_set);
        return *this;
    }

    GraphicsShaderProgram &GraphicsShaderProgram::attach_vertex_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        vertex_shader.shader = shader;
        vertex_shader.entry = entry;
        return *this;
    }

    GraphicsShaderProgram &GraphicsShaderProgram::attach_fragment_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        fragment_shader.shader = shader;
        fragment_shader.entry = entry;
        return *this;
    }

    GraphicsShaderProgram &GraphicsShaderProgram::compile(const GraphicsShaderProgramCompileOptions &options) {
        std::vector<vk::PipelineShaderStageCreateInfo> stages = {
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eVertex, vertex_shader.shader->module.value(), vertex_shader.entry),
            create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eFragment, fragment_shader.shader->module.value(), fragment_shader.entry),
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
            .setLineWidth(options.line_width);

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
            .setStencilTestEnable(options.stencil_test_enable)
            .setFront(options.stencil_front)
            .setBack(options.stencil_back);

        auto locked_renderer = renderer.lock();
        uint32_t subpass_idx = locked_renderer->render_pass_builder->get_subpass_index(subpass_name);
        auto &subpass = *locked_renderer->render_pass_builder->subpass_builders[subpass_idx];

        vk::PipelineColorBlendStateCreateInfo color_blend_state {};
        color_blend_state.setLogicOpEnable(VK_FALSE)
            .setAttachments(subpass.output_attachment_blend_states);

        vk::PipelineDynamicStateCreateInfo dynamic_state {};
        dynamic_state.setDynamicStates(options.dynamic_states);

        compile_pipeline_layout();

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

        pipeline = manager->device->device.createGraphicsPipeline(nullptr, create_info).value;

        return *this;
    }

    GraphicsShaderProgram::~GraphicsShaderProgram() {
        vertex_attribute_set.reset();
        vertex_shader.shader.reset();
        fragment_shader.shader.reset();
    }

    RayTracingShaderProgram::RayTracingShaderProgram() : hit_shader_count(0) {}

    RayTracingShaderProgram &RayTracingShaderProgram::attach_raygen_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        raygen_shader.shader = shader;
        raygen_shader.entry = entry;
        return *this;
    }

    RayTracingShaderProgram &RayTracingShaderProgram::attach_miss_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        miss_shaders.emplace_back(shader, entry);
        return *this;
    }

    RayTracingShaderProgram &RayTracingShaderProgram::attach_hit_group(const ShaderStageInfo &closest_hit_shader, const std::optional<ShaderStageInfo> &intersection_shader) {
        hit_groups.push_back({
            .closest_hit_shader = closest_hit_shader,
            .intersection_shader = intersection_shader,
        });
        hit_shader_count ++;
        if (intersection_shader.has_value()) {
            hit_shader_count ++;
        }
        return *this;
    }

    RayTracingShaderProgram &RayTracingShaderProgram::compile(const RayTracingShaderProgramCompileOptions &options) {
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
        std::vector<vk::PipelineShaderStageCreateInfo> stages;
        shader_groups.reserve(1 + miss_shaders.size() + hit_groups.size());
        stages.reserve(1 + miss_shaders.size() + hit_shader_count);

        shader_groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(stages.size());
        stages.push_back(create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eRaygenKHR, raygen_shader.shader->module.value(), raygen_shader.entry));
        for (auto &miss_shader : miss_shaders) {
            shader_groups.emplace_back()
                .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
                .setGeneralShader(stages.size());
            stages.push_back(create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eMissKHR, miss_shader.shader->module.value(), miss_shader.entry));
        }
        for (auto &hit_group : hit_groups) {
            auto &shader_group = shader_groups.emplace_back()
                .setClosestHitShader(stages.size());
            stages.push_back(create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eClosestHitKHR, hit_group.closest_hit_shader.shader->module.value(), hit_group.closest_hit_shader.entry));
            if (hit_group.intersection_shader.has_value()) {
                shader_group.setIntersectionShader(stages.size())
                    .setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup);
                stages.push_back(create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eIntersectionKHR, hit_group.intersection_shader.value().shader->module.value(), hit_group.intersection_shader.value().entry));
            } else {
                shader_group.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
            }
        }

        compile_pipeline_layout();

        vk::RayTracingPipelineCreateInfoKHR ray_traceing_pipeline_create_info {};
        ray_traceing_pipeline_create_info.setStages(stages)
            .setGroups(shader_groups)
            .setMaxPipelineRayRecursionDepth(options.max_ray_recursion_depth)
            .setLayout(layout);
        pipeline = manager->device->device.createRayTracingPipelineKHR({}, {}, ray_traceing_pipeline_create_info, nullptr, manager->dispatcher).value;

        vk::PhysicalDeviceProperties2 properties {};
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties {};
        properties.pNext = &ray_tracing_pipeline_properties;
        manager->device->physical_device.getProperties2(&properties);

        auto align_address = [](uint32_t size, uint32_t align) {
            return (size + (align - 1)) & ~(align - 1);
        };
        uint32_t handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        uint32_t handle_stride = align_address(handle_size, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);

        raygen_region.setStride(align_address(handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment))
            .setSize(raygen_region.stride);
        miss_region.setStride(handle_stride)
            .setSize(align_address(miss_shaders.size() * handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment));
        hit_region.setStride(handle_stride)
            .setSize(align_address(hit_groups.size() * handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment));

        std::vector<uint8_t> handles_data(handle_size * shader_groups.size());
        vk_assert(manager->device->device.getRayTracingShaderGroupHandlesKHR(pipeline, 0, shader_groups.size(), handles_data.size(), handles_data.data(), manager->dispatcher));
        uint32_t shader_binding_table_size = raygen_region.size + miss_region.size + hit_region.size;
        shader_binding_table_buffer = std::make_unique<Match::Buffer>(shader_binding_table_size, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderBindingTableKHR, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        auto shader_binding_table_buffer_addr = get_buffer_address(shader_binding_table_buffer->buffer);
        raygen_region.setDeviceAddress(shader_binding_table_buffer_addr);
        miss_region.setDeviceAddress(shader_binding_table_buffer_addr + raygen_region.size);
        hit_region.setDeviceAddress(shader_binding_table_buffer_addr + raygen_region.size + miss_region.size);

        auto *ptr = static_cast<uint8_t *>(shader_binding_table_buffer->map());

        memcpy(ptr, handles_data.data(), handle_size);
        ptr += raygen_region.size;
        for (uint32_t i = 0; i < miss_shaders.size(); i ++) {
            memcpy(ptr + i * handle_stride, handles_data.data() + (i + 1) * handle_size, handle_size);
        }
        ptr += miss_region.size;
        for (uint32_t i = 0; i < hit_groups.size(); i ++) {
            memcpy(ptr + i * handle_stride, handles_data.data() + (i + 1 + miss_shaders.size()) * handle_size, handle_size);
        }
        shader_binding_table_buffer->unmap();

        return *this;
    }

    RayTracingShaderProgram::~RayTracingShaderProgram() {
        raygen_shader.shader.reset();
        miss_shaders.clear();
        hit_groups.clear();
        hit_shader_count = 0;
    }

    ComputeShaderProgram::ComputeShaderProgram() {}

    ComputeShaderProgram::~ComputeShaderProgram() {
        compute_shader.shader.reset();
    }

    ComputeShaderProgram &ComputeShaderProgram::attach_compute_shader(std::shared_ptr<Shader> shader, const std::string &entry) {
        compute_shader.shader = shader;
        compute_shader.entry = entry;
        return *this;
    }

    ComputeShaderProgram &ComputeShaderProgram::compile(const ComputeShaderProgramCompileOptions &options) {
        auto shader_stage_create_info = create_pipeline_shader_stage_create_info(vk::ShaderStageFlagBits::eCompute, compute_shader.shader->module.value(), compute_shader.entry);

        compile_pipeline_layout();

        vk::ComputePipelineCreateInfo pipeline_create_info {};
        pipeline_create_info.setLayout(layout)
            .setStage(shader_stage_create_info)
            .setBasePipelineHandle(nullptr)
            .setBasePipelineIndex(0);

        pipeline = manager->device->device.createComputePipeline(nullptr, pipeline_create_info).value;

        return *this;
    }
}
