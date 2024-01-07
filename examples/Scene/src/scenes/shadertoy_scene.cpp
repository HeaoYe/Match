#include "shadertoy_scene.hpp"

void ShaderToyScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    auto &main_subpass = builder->add_subpass("main");
    main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    vertex_buffer = factory->create_vertex_buffer(sizeof(glm::vec2), 4);
    vertex_buffer->upload_data_from_vector<glm::vec2>({
        { 1, 1 },
        { -1, 1 },
        { 1, -1 },
        { -1, -1 },
    });
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint16, 6);
    index_buffer->upload_data_from_vector<uint16_t>({
        0, 1, 2, 1, 2, 3,
    });

    vert_shader = factory->compile_shader("shadertoy/ShaderToy.vert", Match::ShaderStage::eVertex);
    // 所有Shader共用一个PushConstants
    shader_program_constants = factory->create_push_constants(
        Match::ShaderStage::eVertex | Match::ShaderStage::eFragment,
        {
            { "width", Match::ConstantType::eFloat },
            { "height", Match::ConstantType::eFloat },
            { "time", Match::ConstantType::eFloat },
            { "resolution", Match::ConstantType::eFloat2 },
        });
    vas = factory->create_vertex_attribute_set({
        {
            0, Match::InputRate::ePerVertex, { Match::VertexType::eFloat2 },
        }
    });
    shader_program_ds = factory->create_descriptor_set(renderer);
    shader_program_ds->add_descriptors({
        { Match::ShaderStage::eFragment, 0, Match::DescriptorType::eUniform }
    }).allocate();

    shader_input = std::make_unique<ShaderToyInput>(*factory);
    shader_program_ds->bind_uniform(0, shader_input->uniform_buffer);
    updata_shader_program();
}

void ShaderToyScene::updata_shader_program() {    
    auto data = Match::read_binary_file("resource/shaders/shadertoy/ShaderToy.frag");
    auto head_file = std::string(data.data(), data.size());
    data.clear();
    data = Match::read_binary_file("resource/shaders/shadertoy/" + std::string(shader_file_name));
    if (data.empty()) {
        return;
    }
    head_file += std::string(data.data(), data.size());
    data.clear();
    frag_shader.reset();
    frag_shader = factory->compile_shader_from_string(head_file, Match::ShaderStage::eFragment);

    if (!frag_shader->is_ready()) {
        return;
    }

    shader_program.reset();
    shader_program = factory->create_shader_program(renderer, "main");
    shader_program->attach_vertex_shader(vert_shader, "main");
    shader_program->attach_fragment_shader(frag_shader, "main");
    shader_program->attach_vertex_attribute_set(vas);
    shader_program->attach_descriptor_set(shader_program_ds);
    shader_program->attach_push_constants(shader_program_constants);
    shader_program->compile({
        .cull_mode = Match::CullMode::eNone,
        .depth_test_enable = VK_FALSE,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor,
        }
    });

    printf("\n\n\n\n\n\n\n\n");
}

void ShaderToyScene::update(float delta) {
    if (need_update_shader_program) {
        updata_shader_program();
        need_update_shader_program = false;
    }
    time += delta;
    shader_input->uniform->iTimeDelta = delta;
    shader_input->uniform->iTime = time;
    shader_input->uniform->iFrameRate = ImGui::GetIO().Framerate;
    double xpos, ypos;
    static double last_click_x = 0, last_click_y = 0;
    glfwGetCursorPos(Match::window->get_glfw_window(), &xpos, &ypos);
    static bool btn1_down;
    auto btn1 = glfwGetMouseButton(Match::window->get_glfw_window(), GLFW_MOUSE_BUTTON_1);
    shader_input->uniform->iMouse.z = 0;
    shader_input->uniform->iMouse.w = 0;
    if (!btn1_down && btn1 == GLFW_PRESS) {
        btn1_down = true;
    } else if (btn1_down && btn1 == GLFW_RELEASE) {
        btn1_down = false;
        last_click_x = xpos;
        last_click_y = ypos;
        shader_input->uniform->iMouse.w = 1;
    }
    if (btn1_down) {
        shader_input->uniform->iMouse.x = xpos;
        shader_input->uniform->iMouse.y = ypos;
        shader_input->uniform->iMouse.z = 1;
    }
    shader_input->uniform->iDate = glm::vec4(2023, 12, 24, 0);
}

void ShaderToyScene::render() {
    if (frag_shader.get() == nullptr || !frag_shader->is_ready()) {
        return;
    }
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffer(vertex_buffer);
    renderer->bind_index_buffer(index_buffer);
    auto size = Match::runtime_setting->get_window_size();
    float width = size.width;
    float height = size.height;
    shader_program_constants->push_constant("width", width);
    shader_program_constants->push_constant("height", height);
    shader_input->uniform->iResolution = glm::vec3(width, height, 1.0f);
    renderer->set_viewport(0, static_cast<float>(size.height), static_cast<float>(size.width), -static_cast<float>(size.height));
    renderer->set_scissor(0, 0, size.width, size.height);
    renderer->draw_indexed(6, 1, 0, 0, 0);
}

void ShaderToyScene::render_imgui() {
    ImGui::SliderFloat("Time", &time, 0, 128);
    // 在渲染结束前销毁并重建ShaderProgram是错误的
    if (ImGui::Button("编译Shader")) {
        need_update_shader_program = true;
    }
    // 按空格键编译Shader
    if (glfwGetKey(Match::window->get_glfw_window(), GLFW_KEY_SPACE) == GLFW_PRESS) {
        need_update_shader_program = true;
    }
    if (ImGui::InputText("Shader File Name", shader_file_name, 1024)) {
        need_update_shader_program = true;
    }
}

void ShaderToyScene::destroy() {
}

ShaderToyInput::ShaderToyInput(Match::ResourceFactory &factory) {
    uniform_buffer = factory.create_uniform_buffer(sizeof(ShaderToyInputUniform));
    uniform = (ShaderToyInputUniform *) uniform_buffer->get_uniform_ptr();
    memset(uniform, 0, sizeof(ShaderToyInputUniform));
}
