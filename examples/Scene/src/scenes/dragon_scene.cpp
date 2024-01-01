#include "dragon_scene.hpp"
#include <glm/gtx/rotate_vector.hpp>
#include <imgui.h>

void DragonScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    builder->add_attachment("depth", Match::AttachmentType::eDepth);
    builder->add_attachment("PosBuffer", Match::AttachmentType::eFloat4Buffer);
    builder->add_attachment("NormalBuffer", Match::AttachmentType::eFloat4Buffer);
    builder->add_attachment("ColorBuffer", Match::AttachmentType::eFloat4Buffer);

    // 将信息渲染到GBuffer中
    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_output_attachment("PosBuffer");
    main_subpass.attach_output_attachment("NormalBuffer");
    main_subpass.attach_output_attachment("ColorBuffer");
    main_subpass.attach_depth_attachment("depth");

    // 读取GBuffer中数据并渲染
    auto &post_subpass = builder->add_subpass("post");
    post_subpass.attach_input_attachment("PosBuffer");
    post_subpass.attach_input_attachment("NormalBuffer");
    post_subpass.attach_input_attachment("ColorBuffer");
    post_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);

    post_subpass.wait_for(
        "main",
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentRead }, 
        { vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite }
    );
    
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { 0.3f, 0.5f, 0.7f, 1.0f } });

    auto vert_shader = factory->load_shader("dragon_shader/main_subpass.vert", Match::ShaderType::eVertexShaderNeedCompile);
    vert_shader->bind_descriptors({
        { 0, Match::DescriptorType::eUniform },  // Camera
    });
    auto frag_shader = factory->load_shader("dragon_shader/main_subpass.frag", Match::ShaderType::eFragmentShaderNeedCompile);
    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader, "main");
    shader_program->attach_fragment_shader(frag_shader, "main");
    auto vas = factory->create_vertex_attribute_set({
        {
            .binding = 0,
            .rate = Match::InputRate::ePerVertex,
            .attributes = {
                Match::VertexType::eFloat3,  // in_pos
                Match::VertexType::eFloat3,  // in_normal
                Match::VertexType::eFloat3,  // in_color
            }
        },
        {
            .binding = 1,
            .rate = Match::InputRate::ePerInstance,
            .attributes = {
                Match::VertexType::eFloat3,  // offset
            },
        }
    });
    shader_program->bind_vertex_attribute_set(vas);
    shader_program->compile({
        .cull_mode = Match::CullMode::eBack,
        .front_face = Match::FrontFace::eCounterClockwise,
        .depth_test_enable = VK_TRUE,
    });

    auto post_vert_shader = factory->load_shader("dragon_shader/post_subpass.vert", Match::ShaderType::eVertexShaderNeedCompile);
    auto post_frag_shader = factory->load_shader("dragon_shader/post_subpass.frag", Match::ShaderType::eFragmentShaderNeedCompile);
    post_frag_shader->bind_descriptors({
        { 0, Match::DescriptorType::eInputAttachment, 3 },
        { 1, Match::DescriptorType::eUniform },
        { 2, Match::DescriptorType::eUniform },
    });
    post_shader_program = factory->create_shader_program(renderer, "post");
    post_shader_program->attach_vertex_shader(post_vert_shader, "main");
    post_shader_program->attach_fragment_shader(post_frag_shader, "main");
    post_shader_program->compile({
        .cull_mode = Match::CullMode::eNone,
        .depth_test_enable = VK_FALSE,
    });
    auto sampler = factory->create_sampler();
    post_shader_program->bind_input_attachments(
        0,
        { "PosBuffer", "NormalBuffer", "ColorBuffer" },
        { sampler, sampler, sampler }
    );

    camera = std::make_unique<Camera>(*factory);
    shader_program->bind_uniforms(0, { camera->uniform });
    post_shader_program->bind_uniforms(1, { camera->uniform });

    light = std::make_unique<LightController>(factory);
    light->data->lights[0].pos = { 1, 1, 1 };
    light->data->lights[0].color = { 1, 1, 1 };
    light->data->num = 1;
    post_shader_program->bind_uniforms(2, { light->uniform });

    int n = 8, n2 = n / 2;
    offsets.reserve(n * n * n);
    for (int i = 0; i < n * n * n; i ++) {
        offsets.push_back({ i % n - n2, (i / n) % n - n2, ((i / n) / n) % n - n2 });
    }

    camera->data.pos = { 0, 0, -n - 5 };
    camera->upload_data();

    load_model("dragon.obj");
    vertex_buffer = factory->create_vertex_buffer(sizeof(Vertex), vertices.size());
    vertex_buffer->upload_data_from_vector(vertices);
    offset_buffer = factory->create_vertex_buffer(sizeof(glm::vec3), offsets.size());
    offset_buffer->upload_data_from_vector(offsets);
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, indices.size());
    index_buffer->upload_data_from_vector(indices);
}

void DragonScene::update(float delta) {
    camera->update(delta);

    static float time = 0;
    light->data->lights[0].pos = glm::rotateY(glm::vec3(1.0f, 1.0f, 1.0f), time);
    time += delta * 3;
}

void DragonScene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffers({ vertex_buffer, offset_buffer });
    renderer->bind_index_buffer(index_buffer);
    renderer->draw_indexed(indices.size(), offsets.size(), 0, 0, 0);
    renderer->next_subpass();
    renderer->bind_shader_program(post_shader_program);
    renderer->draw_indexed(3, 2, 0, 0, 0);
}

void DragonScene::render_imgui() {
    ImGui::Text("Vertex Count: %d", (int) vertices.size());
    ImGui::Text("Index Count: %d", (int) indices.size());
    // 每一个龙有87w个三角形
    ImGui::Text("Triangle Count: %d", (int) (indices.size() / 3));
    // 512个龙共有4.4亿个三角形
    ImGui::Text("All Triangle Count: %d", (int) ((indices.size() / 3) * offsets.size()));
    ImGui::Text("Current FPS: %f", ImGui::GetIO().Framerate);

    ImGui::Separator();

    ImGui::ColorEdit3("LightColor", &light->data->lights[0].color.x);
}

void DragonScene::destroy() {
}

LightController::LightController(std::weak_ptr<Match::ResourceFactory> factory) {
    uniform = factory.lock()->create_uniform_buffer(sizeof(LightUniform));
    data = (LightUniform *)uniform->get_uniform_ptr();
    memset(data, 0, sizeof(LightUniform));
}
