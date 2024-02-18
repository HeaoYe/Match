#include "pbr_scene.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

void PBRScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    builder->add_attachment("depth", Match::AttachmentType::eDepth);
    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    main_subpass.attach_depth_attachment("depth");
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    auto vert_shader = factory->compile_shader("pbr_shader/shader.vert", Match::ShaderStage::eVertex);
    auto frag_shader = factory->compile_shader("pbr_shader/shader.frag", Match::ShaderStage::eFragment);

    auto shader_program_ds = factory->create_descriptor_set(renderer);
    shader_program_ds->add_descriptors({
        // CameraUniform会被VertexShader和FragmentShader共用，所以将两个ShaderStage | 在一起
        { Match::ShaderStage::eVertex | Match::ShaderStage::eFragment, 0, Match::DescriptorType::eUniform },  // Camera Uniform
        { Match::ShaderStage::eFragment, 1, Match::DescriptorType::eUniform },  // PBR Material
        { Match::ShaderStage::eFragment, 2, Match::DescriptorType::eUniform },  // Light Uniform
    });
    auto vas = factory->create_vertex_attribute_set({
        {
            .binding = 0,
            .rate = Match::InputRate::ePerVertex,
            .attributes = {
                Match::VertexType::eFloat3, Match::VertexType::eFloat3, Match::VertexType::eFloat3
            },
        }
    });

    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader, "main");
    shader_program->attach_fragment_shader(frag_shader, "main");
    shader_program->attach_vertex_attribute_set(vas);
    shader_program->attach_descriptor_set(shader_program_ds);
    shader_program->compile({
        .cull_mode = Match::CullMode::eBack,
        .front_face = Match::FrontFace::eCounterClockwise,
        .depth_test_enable = VK_TRUE,
    });

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 0, -5 };
    camera->upload_data();
    shader_program_ds->bind_uniform(0, camera->uniform);

    material = std::make_unique<PBRMaterial>(*factory);
    material->data->color = glm::vec3(0.5);
    material->data->roughness = 0.5f;
    material->data->metallic = 0.7f;
    material->data->reflectance = 0.7f;
    shader_program_ds->bind_uniform(1, material->uniform_buffer);

    lights = std::make_unique<Lights>(*factory);
    lights->data->color = glm::vec3(1, 1, 1);
    lights->data->direction = glm::normalize(glm::vec3(0, -1, 0));
    shader_program_ds->bind_uniform(2, lights->uniform_buffer);

    model = factory->load_model("mori_knob.obj");
    vertex_buffer = factory->create_vertex_buffer(sizeof(Match::Vertex), model->get_vertex_count());
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, model->get_index_count());
    model->upload_data(vertex_buffer, index_buffer);
}

void PBRScene::update(float delta) {
    camera->update(delta);
    static float time = 0;
    time += delta;
    float n = 1.0f;
    auto pos = glm::rotateY(glm::vec3(n, 0.0f, n), time);
    lights->data->point_lights[0].pos = glm::vec3(0, pos.x, pos.z);    // 绕X轴旋转
    lights->data->point_lights[1].pos = glm::vec3(pos.x, 0, pos.z);    // 绕Y轴旋转
    lights->data->point_lights[2].pos = glm::vec3(pos.x, pos.z, 0);    // 绕Z轴旋转
}

void PBRScene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffer(vertex_buffer);
    renderer->bind_index_buffer(index_buffer);
    renderer->draw_model(model, 1, 0);
}

void PBRScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("平行光颜色", &lights->data->color.r);
    ImGui::ColorEdit3("点光源0颜色", &lights->data->point_lights[0].color.r);
    ImGui::ColorEdit3("点光源1颜色", &lights->data->point_lights[1].color.r);
    ImGui::ColorEdit3("点光源2颜色", &lights->data->point_lights[2].color.r);

    ImGui::ColorEdit3("颜色", &material->data->color.r);
    // ImGui::SliderFloat("粗cao度", &material->data->roughness, 0, 1);
    ImGui::SliderFloat("粗糙度", &material->data->roughness, 0, 1);
    ImGui::SliderFloat("金属度", &material->data->metallic, 0, 1);
    ImGui::SliderFloat("反射度", &material->data->reflectance, 0, 1);
}

void PBRScene::destroy() {
}

PBRMaterial::PBRMaterial(Match::ResourceFactory &factory) {
    uniform_buffer = factory.create_uniform_buffer(sizeof(PBRMaterialUniform));
    data = static_cast<PBRMaterialUniform *>(uniform_buffer->get_uniform_ptr());
    memset(data, 0, sizeof(PBRMaterialUniform));
}

Lights::Lights(Match::ResourceFactory &factory) {
    uniform_buffer = factory.create_uniform_buffer(sizeof(LightsUniform));
    data = static_cast<LightsUniform *>(uniform_buffer->get_uniform_ptr());
    memset(data, 0, sizeof(LightsUniform));
}
