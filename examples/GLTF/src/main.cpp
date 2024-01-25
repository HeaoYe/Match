#include <Match/Match.hpp>
#include "camera.hpp"
#include <imgui.h>

Match::APIManager *ctx;
std::shared_ptr<Match::ResourceFactory> factory;

int main() {
    Match::setting.debug_mode = true;
    Match::setting.window_size = { 1920, 1080 };
    Match::setting.font_size = 26;
    Match::setting.enable_ray_tracing = true;
    Match::set_log_level(Match::LogLevel::eDebug);
    ctx = &Match::Initialize();
    factory = ctx->create_resource_factory("resource");

    {
        auto scene = factory->load_gltf_scene("../../Scene/resource/models/Sponze/glTF/Sponza.gltf");
        auto builder = factory->create_acceleration_structure_builder();
        builder->add_scene(scene);
        builder->build();

        auto bdr = factory->create_render_pass_builder();
        bdr->add_attachment("depth", Match::AttachmentType::eDepth)
            .add_subpass("main")
            .attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT)
            .attach_depth_attachment("depth");
        auto renderer = factory->create_renderer(bdr);
        renderer->attach_render_layer<Match::ImGuiLayer>("imgui");

        auto camera = std::make_unique<Camera>(*factory);

        auto ds = factory->create_descriptor_set(renderer);
        ds->add_descriptors({
            { Match::ShaderStage::eVertex, 0, Match::DescriptorType::eUniform },
        }).allocate()
            .bind_uniform(0, camera->uniform);

        auto vas = factory->create_vertex_attribute_set({
            {
                0, Match::InputRate::ePerVertex,
                { Match::VertexType::eFloat3 }
            }
        });

        auto vert_shader = factory->compile_shader("shader.vert", Match::ShaderStage::eVertex);
        auto frag_shader = factory->compile_shader("shader.frag", Match::ShaderStage::eFragment);
        auto sp = factory->create_shader_program(renderer, "main");
        sp->attach_vertex_shader(vert_shader)
            .attach_fragment_shader(frag_shader)
            .attach_vertex_attribute_set(vas)
            .attach_descriptor_set(ds)
            .compile({
                .cull_mode = Match::CullMode::eBack,
                .front_face = Match::FrontFace::eCounterClockwise,
                .depth_test_enable = VK_TRUE,
            });
        
        auto vertex_buffer = factory->create_vertex_buffer(sizeof(glm::vec3), scene->positions.size());
        vertex_buffer->upload_data_from_vector(scene->positions);
        auto index_buffer = factory->create_index_buffer(Match::IndexType::eUint32, scene->indices.size());
        index_buffer->upload_data_from_vector(scene->indices);

        while (Match::window->is_alive()) {
            Match::window->poll_events();
            camera->update(ImGui::GetIO().DeltaTime);

            renderer->begin_render();

            renderer->bind_shader_program(sp);
            renderer->bind_vertex_buffer(vertex_buffer);
            renderer->bind_index_buffer(index_buffer);
            for (auto *node : scene->all_node_references) {
                for (auto &primitive : node->mesh->primitives) {
                    renderer->draw_indexed(primitive->index_count, 1, primitive->primitive_instance_data.first_index, primitive->primitive_instance_data.first_vertex, 0);
                }
            }

            renderer->begin_layer_render("imgui");
            ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);
            renderer->end_layer_render("imgui");
            renderer->end_render();
        }

        renderer->wait_for_destroy();
    }

    Match::Destroy();
    return 0;
}
