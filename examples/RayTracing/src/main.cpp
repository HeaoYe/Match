#include "utils.hpp"
#include "camera.hpp"
#include <imgui.h>
#include <glm/gtx/rotate_vector.hpp>

std::shared_ptr<Match::VertexBuffer> vertex_buffer;
std::shared_ptr<Match::IndexBuffer> index_buffer;
Match::BufferPosition last_position = { 0, 0 };

struct PointLight {
    alignas(16) glm::vec3 pos;
    alignas(16) glm::vec3 color;
    alignas(4) float intensity;
};

class BLAS {
    no_copy_move_construction(BLAS)
public:
    BLAS(std::shared_ptr<Match::Model> model) : model(model) {
        last_position = model->upload_data(vertex_buffer, index_buffer, last_position);
        vk::AccelerationStructureGeometryKHR geometry;
        vk::AccelerationStructureBuildRangeInfoKHR range;
        uint32_t primitive_count = model->get_index_count() / 3;
        vk::AccelerationStructureGeometryTrianglesDataKHR triangles {};
        triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
            .setVertexStride(sizeof(Match::Vertex))
            .setVertexData(get_address(*vertex_buffer->buffer) + model->position.vertex_buffer_offset * sizeof(Match::Vertex))
            .setMaxVertex(model->vertex_count - 1)
            .setIndexType(vk::IndexType::eUint32)
            .setIndexData(get_address(*index_buffer->buffer) + model->position.index_buffer_offset * Match::transform<uint32_t>(Match::IndexType::eUint32));
        geometry.setGeometry(triangles)
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        range.setFirstVertex(0)
            .setPrimitiveOffset(0)
            .setPrimitiveCount(primitive_count)
            .setTransformOffset(0);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction | vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);

        auto size_info = ctx->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, primitive_count, ctx->dispatcher);

        Match::Buffer scratch_buffer(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        auto scratch_addr = get_address(scratch_buffer);
        
        vk::QueryPoolCreateInfo query_pool_create_info {};
        query_pool_create_info.setQueryCount(1)
            .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
        auto query_pool = ctx->device->device.createQueryPool(query_pool_create_info);
        vk::AccelerationStructureCreateInfoKHR as_create_info {};
        Match::Buffer temp_blas_buffer (size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        as_create_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(temp_blas_buffer.buffer);
        auto temp_blas = ctx->device->device.createAccelerationStructureKHR(as_create_info, nullptr, ctx->dispatcher);

        auto cmd_buf = ctx->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(temp_blas)
            .setScratchData(scratch_addr);
        cmd_buf.buildAccelerationStructuresKHR(build, &range, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        cmd_buf = ctx->command_pool->allocate_single_use();
        cmd_buf.resetQueryPool(query_pool, 0, 1);
        cmd_buf.writeAccelerationStructuresPropertiesKHR(temp_blas, vk::QueryType::eAccelerationStructureCompactedSizeKHR, query_pool, 0, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        auto results = ctx->device->device.getQueryPoolResult<uint32_t>(query_pool, 0, 1, sizeof(vk::DeviceSize), vk::QueryResultFlagBits::eWait);
        auto compacted_size = results.value;
        MCH_INFO("{} -> {}", size_info.accelerationStructureSize, compacted_size);

        buffer = std::make_unique<Match::Buffer>(compacted_size, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        as_create_info.setSize(compacted_size)
            .setBuffer(buffer->buffer);
        blas = ctx->device->device.createAccelerationStructureKHR(as_create_info, nullptr, ctx->dispatcher);
        
        cmd_buf = ctx->command_pool->allocate_single_use();
        vk::CopyAccelerationStructureInfoKHR copy {};
        copy.setSrc(temp_blas)
            .setDst(blas)
            .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
        cmd_buf.copyAccelerationStructureKHR(copy, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);

        ctx->device->device.destroyAccelerationStructureKHR(temp_blas, nullptr, ctx->dispatcher);
        ctx->device->device.destroyQueryPool(query_pool);
    }

    ~BLAS() {
        ctx->device->device.destroyAccelerationStructureKHR(blas, nullptr, ctx->dispatcher);
        buffer.reset();
    }
INNER_VISIBLE:
    vk::AccelerationStructureKHR blas;
    std::unique_ptr<Match::Buffer> buffer;
    std::shared_ptr<Match::Model> model;
};

class TLAS {
    no_copy_move_construction(TLAS)
    struct InstanceInfo {
        vk::DeviceAddress vertex_buffer_address;
        vk::DeviceAddress index_buffer_address;
    };
public:
    TLAS(const std::vector<std::shared_ptr<BLAS>> &blases) {
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        for (auto &blas : blases) {
            instances.emplace_back().setInstanceCustomIndex(instance_infos.size())
                .setTransform(std::array {
                    std::array { 1.0f, 0.0f, 0.0f, 0.0f },
                    std::array { 0.0f, 1.0f, 0.0f, 0.0f },
                    std::array { 0.0f, 0.0f, 1.0f, 0.0f },
                })
                .setAccelerationStructureReference(get_as_address(blas->blas))
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setMask(0xff)
                .setInstanceShaderBindingTableRecordOffset(0);
            instance_infos.push_back({
                .vertex_buffer_address = get_address(*vertex_buffer->buffer) + blas->model->position.vertex_buffer_offset,
                .index_buffer_address = get_address(*index_buffer->buffer) + blas->model->position.index_buffer_offset,
            });
        }
        vk::AccelerationStructureBuildRangeInfoKHR range {};
        range.setPrimitiveCount(blases.size())
            .setPrimitiveOffset(0)
            .setFirstVertex(0)
            .setTransformOffset(0);
        Match::TwoStageBuffer instances_buffer(sizeof(vk::AccelerationStructureInstanceKHR) * instances.size(), vk::BufferUsageFlags(), vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR);
        instance_infos_buffer = std::make_shared<Match::TwoStageBuffer>(instance_infos.size() * sizeof(InstanceInfo), vk::BufferUsageFlagBits::eStorageBuffer);
        instances_buffer.upload_data_from_vector(instances);
        instance_infos_buffer->upload_data_from_vector(instance_infos);

        vk::AccelerationStructureGeometryInstancesDataKHR instance_datas {};
        instance_datas.setData(get_address(*instances_buffer.buffer));
        vk::AccelerationStructureGeometryKHR geometry {};
        geometry.setGeometry(instance_datas)
            .setGeometryType(vk::GeometryTypeKHR::eInstances);

        vk::AccelerationStructureBuildGeometryInfoKHR build {};
        build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
            .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
            .setGeometries(geometry);

        auto size_info = ctx->device->device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice, build, blases.size(), ctx->dispatcher);
        MCH_INFO("Scratch: {}, AS: {}", size_info.buildScratchSize, size_info.accelerationStructureSize)

        vk::AccelerationStructureCreateInfoKHR as_create_info {};
        buffer = std::make_unique<Match::Buffer>(size_info.accelerationStructureSize, vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        as_create_info.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
            .setSize(size_info.accelerationStructureSize)
            .setBuffer(buffer->buffer);
        tlas = ctx->device->device.createAccelerationStructureKHR(as_create_info, nullptr, ctx->dispatcher);

        Match::Buffer scratch_buffer(size_info.buildScratchSize, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        auto scratch_addr = get_address(scratch_buffer);

        auto cmd_buf = ctx->command_pool->allocate_single_use();
        build.setDstAccelerationStructure(tlas)
            .setScratchData(scratch_addr);
        cmd_buf.buildAccelerationStructuresKHR(build, &range, ctx->dispatcher);
        ctx->command_pool->free_single_use(cmd_buf);
    }

    ~TLAS() {
        ctx->device->device.destroyAccelerationStructureKHR(tlas, nullptr, ctx->dispatcher);
        buffer.reset();
    }
INNER_VISIBLE:
    vk::AccelerationStructureKHR tlas;
    std::unique_ptr<Match::Buffer> buffer;
    std::vector<InstanceInfo> instance_infos;
    std::shared_ptr<Match::TwoStageBuffer> instance_infos_buffer;
};

struct GlobalInfo {
    alignas(16) glm::vec2 window_size;
    alignas(16) glm::vec3 clear_color;
};

class RayTracingApplication {
public:
    RayTracingApplication() {
        Match::setting.debug_mode = true;
        Match::setting.max_in_flight_frame = 1;
        Match::setting.window_size = { 1920, 1080 };
        Match::setting.default_font_filename = "/usr/share/fonts/TTF/JetBrainsMonoNerdFontMono-Light.ttf";
        Match::setting.chinese_font_filename = "/usr/share/fonts/adobe-source-han-sans/SourceHanSansCN-Medium.otf";
        Match::setting.font_size = 24.0f;
        Match::setting.enable_ray_tracing = true;
        Match::setting.device_extensions.push_back(VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME);
        ctx = &Match::Initialize();
        Match::runtime_setting->set_vsync(false);
        Match::runtime_setting->set_multisample_count(Match::SampleCount::e1);

        scene_factory = ctx->create_resource_factory("../Scene/resource");
        factory = ctx->create_resource_factory("resource");

        auto builder = factory->create_render_pass_builder();
        builder->add_attachment("depth", Match::AttachmentType::eDepth)
            .add_subpass("main")
            .attach_depth_attachment("depth")
            .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        renderer = factory->create_renderer(builder);
        renderer->attach_render_layer<Match::ImGuiLayer>("imgui");

        auto additional_usages = vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR;
        vertex_buffer = factory->create_vertex_buffer(sizeof(Match::VertexBuffer), 4096000, additional_usages);
        index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, 4096000, additional_usages);

        create_as();

        auto rg = factory->compile_shader("raytrace.rgen", Match::ShaderStage::eRayGen);
        auto miss = factory->compile_shader("raytrace.rmiss", Match::ShaderStage::eMiss);
        auto miss_shadow = factory->compile_shader("raytrace_shadow.rmiss", Match::ShaderStage::eMiss);
        auto closest = factory->compile_shader("raytrace.rchit", Match::ShaderStage::eClosestHit);

        std::vector<vk::PipelineShaderStageCreateInfo> stages(4);
        stages[0].setStage(vk::ShaderStageFlagBits::eRaygenKHR)
            .setModule(rg->module.value())
            .setPName("main");
        stages[1].setStage(vk::ShaderStageFlagBits::eMissKHR)
            .setModule(miss->module.value())
            .setPName("main");
        stages[2].setStage(vk::ShaderStageFlagBits::eMissKHR)
            .setModule(miss_shadow->module.value())
            .setPName("main");
        stages[3].setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
            .setModule(closest->module.value())
            .setPName("main");
        std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups(4);
        shader_groups[0].setGeneralShader(0);
        shader_groups[1].setGeneralShader(1);
        shader_groups[2].setGeneralShader(2);
        shader_groups[3].setClosestHitShader(3)
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);

        ds = factory->create_descriptor_set(renderer);
        ds->add_descriptors({
            { Match::ShaderStage::eRayGen | Match::ShaderStage::eClosestHit, 0, Match::DescriptorType::eAccelerationStructure },
            { Match::ShaderStage::eRayGen, 1, Match::DescriptorType::eStorageImage },
            { Match::ShaderStage::eClosestHit, 2, Match::DescriptorType::eStorageBuffer },
        }).allocate();

        shared_ds = factory->create_descriptor_set(renderer);
        shared_ds->add_descriptors({
            { Match::ShaderStage::eFragment | Match::ShaderStage::eRayGen, 0, Match::DescriptorType::eUniform },
            { Match::ShaderStage::eVertex | Match::ShaderStage::eRayGen, 1, Match::DescriptorType::eUniform },
            { Match::ShaderStage::eFragment | Match::ShaderStage::eClosestHit, 2, Match::DescriptorType::eUniform },
        }).allocate();

        auto [width, height] = Match::runtime_setting->get_window_size();
        image = factory->create_storage_image(width * scale, height * scale);
        ds->bind_storage_image(1, image);
        ds->bind_storage_buffer(2, tlas->instance_infos_buffer);
        callback_id = renderer->register_resource_recreate_callback([this]() {
            image.reset();
            auto [width, height] = Match::runtime_setting->get_window_size();
            MCH_INFO("Recreate {} {}", width, height)
            image = factory->create_storage_image(width * scale, height * scale);
            ds->bind_storage_image(1, image);
            ds_ra->bind_texture(0, image, sampler);
            camera->update_aspect();
            camera->upload_data();
        });

        vk::WriteDescriptorSetAccelerationStructureKHR tlas_write {};
        tlas_write.setAccelerationStructures(tlas->tlas);
        for (uint32_t in_flight = 0; in_flight < Match::setting.max_in_flight_frame; in_flight ++) {
        vk::WriteDescriptorSet write {};
            write.setDstSet(ds->descriptor_sets[in_flight])
                .setDstBinding(0)
                .setDstArrayElement(0)
                .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                .setDescriptorCount(1)
                .setPNext(&tlas_write);
            ctx->device->device.updateDescriptorSets(write, {});
        }

        vk::PipelineLayoutCreateInfo layout_create_info {};
        auto layouts = { ds->descriptor_layout, shared_ds->descriptor_layout };
        layout_create_info.setSetLayouts(layouts);
        layout = ctx->device->device.createPipelineLayout(layout_create_info);

        vk::RayTracingPipelineCreateInfoKHR pipeline_create_info {};
        pipeline_create_info.setStages(stages)
            .setGroups(shader_groups)
            .setMaxPipelineRayRecursionDepth(2)
            .setLayout(layout);
        ray_trace_pipeline = ctx->device->device.createRayTracingPipelineKHR({}, {}, pipeline_create_info, nullptr, ctx->dispatcher).value;

        vk::PhysicalDeviceProperties2 properties {};
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR acceleration_structure_properties {};
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR ray_tracing_pipeline_properties {};
        properties.pNext = &acceleration_structure_properties;
        acceleration_structure_properties.pNext = &ray_tracing_pipeline_properties;
        ctx->device->physical_device.getProperties2(&properties);

        auto aligned = [](uint32_t size, uint32_t align) {
            return (size + (align - 1)) & ~(align - 1);
        };
        uint32_t handle_size = ray_tracing_pipeline_properties.shaderGroupHandleSize;
        uint32_t handle_stride = aligned(handle_size, ray_tracing_pipeline_properties.shaderGroupHandleAlignment);
        MCH_WARN("{}", handle_stride)
        
        rgen_region.setStride(aligned(handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment))
            .setSize(rgen_region.stride);
        miss_region.setStride(handle_stride)
            .setSize(aligned(2 * handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment));
        hit_region.setStride(handle_stride)
            .setSize(aligned(handle_stride, ray_tracing_pipeline_properties.shaderGroupBaseAlignment));
        std::vector<uint8_t> handles_data(handle_size * shader_groups.size());
        vk_assert(ctx->device->device.getRayTracingShaderGroupHandlesKHR(ray_trace_pipeline, 0, shader_groups.size(), handle_size * shader_groups.size(), handles_data.data(), ctx->dispatcher));
        uint32_t sbt_size = rgen_region.size + miss_region.size + hit_region.size;
        sbt_buffer = std::make_unique<Match::Buffer>(sbt_size, vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderBindingTableKHR, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        auto sbt_buffer_addr = get_address(*sbt_buffer);
        rgen_region.setDeviceAddress(sbt_buffer_addr);
        miss_region.setDeviceAddress(sbt_buffer_addr + rgen_region.size);
        hit_region.setDeviceAddress(sbt_buffer_addr + rgen_region.size + miss_region.size);
        auto *ptr = static_cast<uint8_t *>(sbt_buffer->map());

        memcpy(ptr, handles_data.data(), handle_size);
        ptr += rgen_region.size;
        for (uint32_t i = 0; i < 2; i ++) {
            memcpy(ptr + i * handle_stride, handles_data.data() + (i + 1) * handle_size, handle_size);
        }
        ptr += miss_region.size;
        memcpy(ptr, handles_data.data() + 3 * handle_size, handle_size);
        sbt_buffer->unmap();

        sp = factory->create_shader_program(renderer, "main");
        auto vs = factory->compile_shader("shader.vert", Match::ShaderStage::eVertex);
        auto fs = factory->compile_shader("shader.frag", Match::ShaderStage::eFragment);
        ds_ra = factory->create_descriptor_set(renderer);
        ds_ra->add_descriptors({  
            { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eTexture },
        });
        sp->attach_vertex_shader(vs)
            .attach_fragment_shader(fs)
            .attach_descriptor_set(ds_ra)
            .attach_descriptor_set(shared_ds, 1)
            .compile({
                .cull_mode = Match::CullMode::eNone,
                .depth_test_enable = VK_FALSE,
                .dynamic_states = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor,
                }
            });
        sampler = factory->create_sampler({
            .min_filter = Match::SamplerFilter::eLinear,
        });
        ds_ra->bind_texture(0, image, sampler);
        g = factory->create_uniform_buffer(sizeof(GlobalInfo));
        light = factory->create_uniform_buffer(sizeof(PointLight));
        shared_ds->bind_uniform(0, g);
        shared_ds->bind_uniform(2, light);
        g_info = static_cast<GlobalInfo *>(g->get_uniform_ptr());
        light_ptr = static_cast<PointLight *>(light->get_uniform_ptr());
        light_ptr->pos = { 2, 2, 2 };
        light_ptr->color = { 1, 1, 1 };
        light_ptr->intensity = 1;
        camera = std::make_unique<Camera>(*factory);
        camera->data.pos = { 0, 0, -4 };
        camera->upload_data();
        shared_ds->bind_uniform(1, camera->uniform);

        auto vas = factory->create_vertex_attribute_set({
            Match::Vertex::generate_input_binding(0)
        });
        raster_sp = factory->create_shader_program(renderer, "main");
        raster_sp->attach_vertex_shader(factory->compile_shader("raster.vert", Match::ShaderStage::eVertex))
            .attach_fragment_shader(factory->compile_shader("raster.frag", Match::ShaderStage::eFragment))
            .attach_vertex_attribute_set(vas)
            .attach_descriptor_set(shared_ds)
            .compile({
                .cull_mode = Match::CullMode::eBack,
                .front_face = Match::FrontFace::eCounterClockwise,
                .depth_test_enable = VK_TRUE,
                .dynamic_states = {
                    vk::DynamicState::eViewport,
                    vk::DynamicState::eScissor,
                }
            });
    }

    ~RayTracingApplication() {
        renderer->wait_for_destroy();
        blas.reset();
        blas_ground.reset();
        tlas.reset();
        camera.reset();
        sbt_buffer.reset();
        renderer->remove_resource_recreate_callback(callback_id);
        image.reset();
        ctx->device->device.destroyPipeline(ray_trace_pipeline);
        ctx->device->device.destroyPipelineLayout(layout);
        ds.reset();
        ds_ra.reset();
        shared_ds.reset();
        g.reset();
        light.reset();
        sampler.reset();
        sp.reset();
        raster_sp.reset();
        renderer.reset();
        vertex_buffer.reset();
        index_buffer.reset();
        Match::Destroy();
    }

    void create_as() {
        blas = std::make_shared<BLAS>(scene_factory->load_model("dragon.obj"));
        // blas = std::make_shared<BLAS>(factory->load_model("cube.obj"));
        tlas = std::make_unique<TLAS>(std::vector<std::shared_ptr<BLAS>>({ blas }));
    }

    void gameloop() {
        while (Match::window->is_alive()) {
            Match::window->poll_events();
            camera->update(ImGui::GetIO().DeltaTime);
            static float time = 0;
            time += ImGui::GetIO().DeltaTime;
            light_ptr->pos = glm::rotateY(glm::vec3(2, light_ptr->pos.y, 2), time);
            renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { g_info->clear_color.r, g_info->clear_color.g, g_info->clear_color.b, 1.0f } });

            renderer->acquire_next_image();

            if (is_ray_tracing) {
                auto cmd_buf = renderer->get_command_buffer();

                auto [width, height] = Match::runtime_setting->get_window_size();
                g_info->window_size = { static_cast<float>(width), static_cast<float>(height) };
                
                cmd_buf.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, ray_trace_pipeline);
                cmd_buf.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, layout, 0, { ds->descriptor_sets[renderer->current_in_flight], shared_ds->descriptor_sets[renderer->current_in_flight] }, {});
                cmd_buf.traceRaysKHR(rgen_region, miss_region, hit_region, call_region, width * scale, height * scale, 1, ctx->dispatcher);

                renderer->begin_render_pass();

                renderer->bind_shader_program(sp);
                renderer->set_scissor(0, 0, width, height);
                renderer->set_viewport(0, height, width, -static_cast<float>(height));
                renderer->draw(3, 2, 0, 0);
            } else {
                renderer->begin_render_pass();

                auto [width, height] = Match::runtime_setting->get_window_size();
                renderer->bind_shader_program(raster_sp);
                renderer->set_scissor(0, 0, width, height);
                renderer->set_viewport(0, height, width, -static_cast<float>(height));
                renderer->bind_vertex_buffer(vertex_buffer);
                renderer->bind_index_buffer(index_buffer);
                renderer->draw_model(blas->model, 1, 0);
            }

            renderer->begin_layer_render("imgui");
            ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
            ImGui::Checkbox("Enabel Ray Tracing", &is_ray_tracing);
            ImGui::SliderFloat("Ray Tracing Scale", &scale, 1, 5);
            ImGui::SliderFloat3("Clear Color", &g_info->clear_color.r, 0, 1);
            ImGui::SliderFloat3("Ligjt Color", &light_ptr->color.r, 0, 1);
            ImGui::SliderFloat("Ligjt Intensity", &light_ptr->intensity, 0, 10);
            ImGui::SliderFloat("Ligjt Height", &light_ptr->pos.y, -5, 5);
            renderer->end_layer_render("imgui");

            renderer->end_render_pass();

            renderer->present();
        }
    }
private:
    std::shared_ptr<BLAS> blas;
    std::shared_ptr<BLAS> blas_ground;
    std::unique_ptr<TLAS> tlas;

    std::shared_ptr<Match::ResourceFactory> scene_factory;
    std::shared_ptr<Match::ResourceFactory> factory;
    std::shared_ptr<Match::Renderer> renderer;
    std::shared_ptr<Match::DescriptorSet> ds;
    std::shared_ptr<Match::Sampler> sampler;
    std::shared_ptr<Match::ShaderProgram> sp;
    std::shared_ptr<Match::DescriptorSet> ds_ra;
    std::shared_ptr<Match::DescriptorSet> shared_ds;
    std::shared_ptr<Match::StorageImage> image;
    std::shared_ptr<Match::UniformBuffer> g;
    GlobalInfo *g_info;
    std::shared_ptr<Match::UniformBuffer> light;
    PointLight *light_ptr;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Match::Buffer> sbt_buffer;
    uint32_t callback_id;
    vk::StridedDeviceAddressRegionKHR rgen_region {};
    vk::StridedDeviceAddressRegionKHR miss_region {};
    vk::StridedDeviceAddressRegionKHR hit_region {};
    vk::StridedDeviceAddressRegionKHR call_region {};
    vk::PipelineLayout layout;
    vk::Pipeline ray_trace_pipeline;

    bool is_ray_tracing = false;
    float scale = 2;
    std::shared_ptr<Match::ShaderProgram> raster_sp;
};

int main() {
    RayTracingApplication app {};
    // 刚做完底层加速结构和顶层加速结构的创建。。。。
    // 一个简单的光追场景，包括一个模型，一个点光源，以及阴影
    // 未来会集成到渲染引擎中
    app.gameloop();
}
