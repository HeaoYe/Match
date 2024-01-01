#include "model_viewer_scene.hpp"

void ModelViewerScene::initialize() {
    auto builder = factory->create_render_pass_builder();
    builder->add_attachment("depth", Match::AttachmentType::eDepth)
        .add_subpass("main")
        .attach_depth_attachment("depth")
        .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
    renderer = factory->create_renderer(builder);
    renderer->attach_render_layer<Match::ImGuiLayer>("imgui layer");

    camera = std::make_unique<Camera>(*factory);
    camera->data.pos = { 0, 0, -3 };
    camera->upload_data();
    
    shader_program = factory->create_shader_program(renderer, "main");
    auto vert_shader = factory->load_shader("model_viewer_shader/shader.vert", Match::ShaderType::eVertexShaderNeedCompile);
    vert_shader->bind_descriptors({
        { 0, Match::DescriptorType::eUniform },
    });
    auto frag_shader = factory->load_shader("model_viewer_shader/shader.frag", Match::ShaderType::eFragmentShaderNeedCompile);
    auto vas = factory->create_vertex_attribute_set({
         Match::Vertex::generate_input_binding(0),
         {
            .binding = 1,
            .rate = Match::InputRate::ePerInstance,
            .attributes = { Match::VertexType::eFloat3, Match::VertexType::eFloat }
         }
    });
    shader_program->attach_vertex_shader(vert_shader)
        .attach_fragment_shader(frag_shader)
        .bind_vertex_attribute_set(vas)
        .compile({
            .cull_mode = Match::CullMode::eBack,
            .front_face = Match::FrontFace::eCounterClockwise,
            .depth_test_enable = VK_TRUE,
        })
        .bind_uniforms(0, { camera->uniform });
    
    vertex_buffer = factory->create_vertex_buffer(sizeof(Match::Vertex), 4096000);
    index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, 4096000);
}

void ModelViewerScene::update(float dt) {
    camera->update(dt);
}

void ModelViewerScene::render() {
    renderer->bind_shader_program(shader_program);
    renderer->bind_vertex_buffer(vertex_buffer);
    renderer->bind_index_buffer(index_buffer);
    for (auto &[filename, model_info] : model_infos) {
        model_info.instances_vertex_buffer->upload_data_from_vector(model_info.instances_info);
        renderer->bind_vertex_buffer(model_info.instances_vertex_buffer, 1);
        for (auto &[mesh_name, visiable] : model_info.meshes_visibale) {
            if (!visiable) {
                continue;
            }
            renderer->draw_model_mesh(model_info.model, mesh_name, model_info.instances_info.size(), 0);
        }
    }
}

void ModelViewerScene::render_imgui() {
    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    static char filename[1024] = "mori_knob.obj";
    static char instance_name[1024] = { 0 };
    static Match::BufferPosition last_upload_position = { 0, 0 };

    ImGui::InputText("模型文件名", filename, 1024);
    ImGui::InputText("实例名", instance_name, 1024);
    if (ImGui::Button("加载模型")) {
        auto model_pos = model_infos.find(filename);
        if (model_pos == model_infos.end()) {
            auto model = factory->load_model(filename);
            last_upload_position = model->upload_data(vertex_buffer, index_buffer, last_upload_position);
            model_infos.insert(std::make_pair(
                filename,
                ModelInfo {
                    model,
                    factory->create_vertex_buffer(sizeof(InstanceInfo), 512),
                    {},
                    {},
                }
            ));
            MCH_WARN("加载模型：{}", filename)
        }
        
        auto instance_pos = instance_querys.find(instance_name);
        if (instance_pos == instance_querys.end()) {
            instance_querys.insert(std::make_pair(instance_name, std::make_pair(filename, model_infos[filename].instances_info.size())));
            model_infos[filename].instances_info.push_back({
                .offset = { 0, 0, 0 },
                .scale = 1.0f,
            });
        } else {
            MCH_ERROR("已存在名为\"{}\"的实例", instance_name);
        }
    }

    ImGui::SeparatorText("Instance Selector");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, -1.0f });
    static int current_selected_num = -1;
    int i = 0;
    static std::string active_instance_name;
    for (auto &[name, info] : instance_querys) {
        if (ImGui::RadioButton(name.c_str(), &current_selected_num, i)) {
            active_instance_name = name;
        }
        i += 1;
    }
    ImGui::PopStyleVar();

    ImGui::SeparatorText("Instance Info Editor");
    if (!active_instance_name.empty()) {
        auto &query = instance_querys[active_instance_name];
        auto &instance_info = model_infos[query.first].instances_info[query.second];
        ImGui::SliderFloat3("Offset", &instance_info.offset.x, -10, 10);
        ImGui::SliderFloat("Scale", &instance_info.scale, 0.0001, 10);
    }

    ImGui::SeparatorText("Model Mesh Editor");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0, -1.0f });
    for (auto &[model_filename, model_info] : model_infos) {
        ImGui::Text("%s Meshes", model_filename.c_str());
        for (auto &mesh_name : model_info.model->enumerate_meshes_name()) {
            ImGui::Checkbox(mesh_name.c_str(), &model_info.meshes_visibale[mesh_name]);
        }
        ImGui::Separator();
    }
    ImGui::PopStyleVar();
}

void ModelViewerScene::destroy() {
}

