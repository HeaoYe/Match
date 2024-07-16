#include "ssao_scene.hpp"
#include <random>

void SSAOScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    builder->add_attachment("depth", Match::AttachmentType::eDepth);
    // PosBuffer 前三个Float存储世界坐标系坐标，最后一个Float存储深度信息
    builder->add_attachment("PosBuffer", Match::AttachmentType::eFloat4Buffer);
    // NormalBuffer 前三个Float存储世界坐标系法向量，最后一个Float标记该位置是否有Fragment，有为1，没有为0
    builder->add_attachment("NormalBuffer", Match::AttachmentType::eFloat4Buffer);
    builder->add_attachment("SSAOBuffer", Match::AttachmentType::eFloat4Buffer);

    auto &prepare_subpass = builder->add_subpass("prepare");
    prepare_subpass.attach_output_attachment("PosBuffer");
    prepare_subpass.attach_output_attachment("NormalBuffer");
    prepare_subpass.attach_depth_attachment("depth");

    auto &ssao_subpass = builder->add_subpass("ssao");
    ssao_subpass.attach_input_attachment("PosBuffer");
    ssao_subpass.attach_input_attachment("NormalBuffer");
    ssao_subpass.attach_output_attachment("SSAOBuffer");
    ssao_subpass.wait_for(
        "prepare",
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentRead },
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
    );

    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_input_attachment("PosBuffer");
    main_subpass.attach_input_attachment("NormalBuffer");
    main_subpass.attach_input_attachment("SSAOBuffer");
    main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    main_subpass.wait_for(
        "prepare",
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentRead },
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
    );

    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");
    // 设置NormalBuffer的ClearValue，将最后一个Float清零
    renderer->set_clear_value("NormalBuffer", { { 0.0f, 0.0f, 0.0f, 0.0f } });

    auto prepare_shader_program_ds = factory->create_descriptor_set(renderer);
    auto ssao_shader_program_ds = factory->create_descriptor_set(renderer);
    auto main_shader_program_ds = factory->create_descriptor_set(renderer);
    prepare_shader_program = factory->create_shader_program(renderer, "prepare");
    ssao_shader_program = factory->create_shader_program(renderer, "ssao");
    main_shader_program = factory->create_shader_program(renderer, "main");

    auto prepare_vert_shader = factory->compile_shader("ssao_shader/prepare.vert", Match::ShaderStage::eVertex);
    auto prepare_frag_shader = factory->compile_shader("ssao_shader/prepare.frag", Match::ShaderStage::eFragment);
    auto deferred_vert_shader = factory->compile_shader("ssao_shader/deferred.vert", Match::ShaderStage::eVertex);
    auto ssao_frag_shader = factory->compile_shader("ssao_shader/ssao.frag", Match::ShaderStage::eFragment);
    auto main_frag_shader = factory->compile_shader("ssao_shader/main.frag", Match::ShaderStage::eFragment);
    auto vas = factory->create_vertex_attribute_set({
        {
            .binding = 0,
            .rate = Match::InputRate::ePerVertex,
            .attributes = {
                Match::VertexType::eFloat3,  // in_pos
                Match::VertexType::eFloat3,  // in_normal
                Match::VertexType::eFloat3,  // in_color
            }
        }
    });
    Match::GraphicsShaderProgramCompileOptions deferred_option {
        .cull_mode = Match::CullMode::eNone,
        .depth_test_enable = VK_FALSE,
    };

    prepare_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eVertex, 0, Match::DescriptorType::eUniform },
    });
    ssao_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eTextureAttachment },
        { Match::ShaderStage::eFragment, 1, Match::DescriptorType::eInputAttachment },
        { Match::ShaderStage::eFragment, 2, Match::DescriptorType::eUniform },
        { Match::ShaderStage::eFragment, 3, Match::DescriptorType::eTexture },
        { Match::ShaderStage::eFragment, 4, Match::DescriptorType::eUniform },
    });
    ssao_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eFragment,
        {
            { "window_size", Match::ConstantType::eFloat2 },
            { "a", Match::ConstantType::eInt32 },
            { "sample_count", Match::ConstantType::eInt32 },
            { "r", Match::ConstantType::eFloat },
        });
    main_shader_program_ds->add_descriptors({
        { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eInputAttachment },
        { Match::ShaderStage::eFragment, 1, Match::DescriptorType::eInputAttachment },
        { Match::ShaderStage::eFragment, 2, Match::DescriptorType::eTextureAttachment },
    });
    main_shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eFragment,
        {
            { "window_size", Match::ConstantType::eFloat2 },
            { "ssao_enable", Match::ConstantType::eInt32 },
            { "blur_kernel_size", Match::ConstantType::eInt32 },
        });

    // entry默认为"main"
    prepare_shader_program->attach_vertex_shader(prepare_vert_shader);
    prepare_shader_program->attach_fragment_shader(prepare_frag_shader);
    prepare_shader_program->attach_vertex_attribute_set(vas);
    prepare_shader_program->attach_descriptor_set(prepare_shader_program_ds);
    prepare_shader_program->compile({
        .cull_mode = Match::CullMode::eBack,
        .front_face = Match::FrontFace::eCounterClockwise,
        .depth_test_enable = VK_TRUE,
    });

    ssao_shader_program->attach_vertex_shader(deferred_vert_shader);
    ssao_shader_program->attach_fragment_shader(ssao_frag_shader);
    ssao_shader_program->attach_descriptor_set(ssao_shader_program_ds);
    ssao_shader_program->attach_push_constants(ssao_shader_program_constants);
    ssao_shader_program->compile(deferred_option);

    main_shader_program->attach_vertex_shader(deferred_vert_shader);
    main_shader_program->attach_fragment_shader(main_frag_shader);
    main_shader_program->attach_descriptor_set(main_shader_program_ds);
    main_shader_program->attach_push_constants(main_shader_program_constants);
    main_shader_program->compile(deferred_option);

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 0, -3 };
    camera->upload_data();
    prepare_shader_program_ds->bind_uniform(0, camera->uniform);

    auto sampler = factory->create_sampler({
        .address_mode_u = Match::SamplerAddressMode::eRepeat,
        .address_mode_v = Match::SamplerAddressMode::eRepeat,
    });
    ssao_shader_program_ds->bind_input_attachment(0, "PosBuffer", sampler);
    ssao_shader_program_ds->bind_input_attachment(1, "NormalBuffer", sampler);
    main_shader_program_ds->bind_input_attachment(0, "PosBuffer", sampler);
    main_shader_program_ds->bind_input_attachment(1, "NormalBuffer", sampler);
    main_shader_program_ds->bind_input_attachment(2, "SSAOBuffer", sampler);

    // 加载模型
    // model = factory->load_model("dragon.obj");
    model = factory->load_model("mori_knob.obj");
    // 创建buffer
    vertex_buffer = factory->create_vertex_buffer(sizeof(Match::Vertex), model->get_vertex_count());
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, model->get_index_count());
    // 上传数据
    model->upload_data(vertex_buffer, index_buffer);

    std::uniform_real_distribution<float> float_distribution(0, 1);
    std::default_random_engine engine;
    std::vector<glm::vec4> ssao_samples(64);
    // 随机旋转纹理的边长
    const int a = 4;
    std::vector<glm::vec4> ssao_random_vecs(a * a);
    ssao_shader_program_constants->push_constant("a", a);

    for (auto &sample : ssao_samples) {
        sample = glm::normalize(glm::vec4(
            // [0, 1] --> [-1, 1]
            float_distribution(engine) * 2 - 1,
            // [0, 1] --> [-1, 1]
            float_distribution(engine) * 2 - 1,
            // [0, 1] --> [0.3, 1]
            float_distribution(engine) * 0.7 + 0.3,
            0
        ));
        // 让sample更多的分布在原点附近
        float scale = float_distribution(engine) * 0.5 + 0.5;
        scale = 0.1 + 0.9 * scale * scale;
        sample *= scale;
    }
    for (auto &random_vec : ssao_random_vecs) {
        random_vec = glm::normalize(glm::vec4(
            float_distribution(engine) * 2 - 1,
            float_distribution(engine) * 2 - 1,
            0, 0
        ));
    }
    ssao_samples_uniform_buffer = factory->create_uniform_buffer(sizeof(glm::vec4) * ssao_samples.size());
    memcpy(ssao_samples_uniform_buffer->get_uniform_ptr(), ssao_samples.data(), sizeof(glm::vec4) * ssao_samples.size());
    ssao_random_vecs_texture = factory->create_texture(reinterpret_cast<uint8_t *>(ssao_random_vecs.data()), a, a);

    ssao_shader_program_ds->bind_uniform(2, ssao_samples_uniform_buffer);
    // ssao_random_vecs_texture的sampler的address_mode_uv需要设置为Repeat
    ssao_shader_program_ds->bind_texture(3, ssao_random_vecs_texture, sampler);
    ssao_shader_program_ds->bind_uniform(4, camera->uniform);
}

void SSAOScene::update(float dt) {
    camera->update(dt);
    auto usize = Match::runtime_setting->get_window_size();
    glm::vec2 size {
        static_cast<float>(usize.width),
        static_cast<float>(usize.height),
    };
    ssao_shader_program_constants->push_constant("window_size", &size);
    main_shader_program_constants->push_constant("window_size", &size);
}

void SSAOScene::render() {
    renderer->bind_shader_program(prepare_shader_program);
    renderer->bind_vertex_buffer(vertex_buffer);
    renderer->bind_index_buffer(index_buffer);
    // 绘制模型
    renderer->draw_model(model, 1, 0);

    renderer->next_subpass();
    renderer->bind_shader_program(ssao_shader_program);
    renderer->draw_indexed(3, 2, 0, 0, 0);

    renderer->next_subpass();
    renderer->bind_shader_program(main_shader_program);
    renderer->draw_indexed(3, 2, 0, 0, 0);
}

void SSAOScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    static int sample_count = 32;
    ImGui::SliderInt("Sample Count", &sample_count, 1, 64);
    ssao_shader_program_constants->push_constant("sample_count", sample_count);

    static float r = 0.4f;
    ImGui::SliderFloat("R", &r, 0.01, 10);
    ssao_shader_program_constants->push_constant("r", r);

    static int blur_kernel_size = 4;
    ImGui::SliderInt("Blur Kernel Size", &blur_kernel_size, 1, 16);
    main_shader_program_constants->push_constant("blur_kernel_size", blur_kernel_size);

    static bool ssao_enable = 0;
    ImGui::Checkbox("Enable SSAO", &ssao_enable);
    main_shader_program_constants->push_constant("ssao_enable", static_cast<int>(ssao_enable));
}

void SSAOScene::destroy() {
}
