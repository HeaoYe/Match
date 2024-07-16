#include <Match/Match.hpp>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

int main() {
    // 配置Match
    Match::setting.debug_mode = true;
    Match::setting.device_name = Match::AUTO_SELECT_DEVICE;
    Match::setting.app_name = "App Name";
    Match::setting.chinese_font_filename = "../fonts/SourceHanSansCN-Normal.ttf";
#if !defined(PLATFORM_WINDOWS)
    Match::setting.default_font_filename = "/usr/share/fonts/TTF/JetBrainsMonoNerdFontMono-Light.ttf";
#endif
    Match::setting.font_size = 26.0f;

    // 初始化Match
    auto &context = Match::Initialize();
    // 启用MSAA
    Match::runtime_setting->set_multisample_count(Match::SampleCount::e8);
    // 禁用MSAA
    // Match::runtime_setting->set_multisample_count(VK_SAMPLE_COUNT_1_BIT);
    // 显示全部设备名，可填写在Match::setting.device_name中
    // Match::EnumerateDevices();

    {
        // 初始化资源工厂，会在${root}/shaders文件夹下寻找Shader文件，，会在${root}/textures文件夹下寻找图片文件
        auto factory = context.create_resource_factory("./resource");

        // 配置Vulkan的RenderPass
        auto builder = factory->create_render_pass_builder();
        // Attchment --> Attachment   之前拼错了
        // RenderPassBuilder在创建时会自动添加SWAPCHAIN_IMAGE_ATTACHMENT作为PresentSrc
        // builder->add_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, Match::AttachmentType::eColor);
        builder->add_attachment("Depth Buffer", Match::AttachmentType::eDepth);

        // 创建Subpass
        auto &subpass = builder->add_subpass("MainSubpass");
        // 绑定Subpass为图形流水线
        subpass.bind(vk::PipelineBindPoint::eGraphics);
        // 设置Subpass的输出为SWAPCHAIN_IMAGE_ATTACHMENT
        subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        subpass.attach_depth_attachment("Depth Buffer");
        // 添加Subpass Dependency
        subpass.wait_for(
            Match::EXTERNAL_SUBPASS,
            { vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite },
            { vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests, vk::AccessFlagBits::eNone }
        );
        // 将RenderPass与Renderer绑定
        // 改为使用ResourceFactory创建Renderer
        auto renderer = factory->create_renderer(builder);
        // 为renderer添加ImGui渲染层
        // 会在所有Subpass后添加一个渲染ImGui的Subpass
        // 所以渲染ImGui的操作要放到其他渲染的后面
        renderer->attach_render_layer<Match::ImGuiLayer>("ImGui");

        // 配置ImGui
        ImGui::StyleColorsDark();
        // auto &style = ImGui::GetStyle();
        // style.FramePadding = { 10, 50 };

        // 加载已编译的Shader文件
        // auto vert_shader = factory->load_shader("vert.spv");
        // auto frag_shader = factory->load_shader("frag.spv");
        // 动态编译Shader文件
        // 加载预编译的Shader
        // factory->load_shader("xxx.spv");
        // 编译Shader文件
        // factory->compile_shader("xxx.vert", ShaderStage)
        auto vert_shader = factory->compile_shader("shader.vert", Match::ShaderStage::eVertex);
        auto frag_shader = factory->compile_shader("shader.frag", Match::ShaderStage::eFragment);

        // 创建顶点数据结构体
        struct Vertex {
            glm::vec3 pos;
            glm::vec3 color;
            glm::vec2 uv;
        };
        // 顶点数据
        // 因为没有使用投影矩阵变换坐标，所以这里的z分量是指深度，右上角的点深度是0.1，显示在最上层，右下角的点深度是0.3，显示在最下层
        const std::vector<Vertex> vertices = {
            { { 0.5f, 0.5f, 0.1f }, { 0.5f, 0.2f, 0.2f }, { 1.0f, 1.0f } },
            { { 0.5f, -0.5f, 0.2f }, { 0.2f, 0.5f, 0.2f }, { 1.0f, 0.0f } },
            { { -0.5f, 0.5f, 0.2f }, { 0.2f, 0.5f, 0.2f }, { 0.0f, 1.0f } },
            { { -0.5f, -0.5f, 0.3f }, { 0.2f, 0.2f, 0.5f }, { 0.0f, 0.0f } },
        };
        // 每个实例的offset
        // 渲染顺序与offsets的顺序有关
        // 启用深度缓存后，覆盖只与深度有关，与渲染顺序无关
        // 因为depth的默认深度是1，任何比深度1大的都会被深度测试剔除
        const std::vector<glm::vec3> offsets = {
            // 第一象限的实例深度最少，在最上层
            { 0.5f, 0.5f, 1.0f },   // 第一象限
            // 第二象限和第四象限的实例被第一象限的覆盖
            { -0.5f, 0.5f, 2.0f },  // 第二象限
            { 0.5f, -0.5f, 2.0f },  // 第四象限
            // 第三象限的实例在最下层
            { -0.5f, -0.5f, 3.0f }, // 第三象限
        };
        const std::vector<uint16_t> indices = {
            0, 1, 2, 1, 2, 3  // 绘制正方形
        };
        struct PosScaler {
            float x_pos_scale;
            float y_pos_scale;
        };
        struct ColorScaler {
            float color_scale;
        };

        // 创建顶点数据描述符
        // 重写了VertexAttribute的构造过程
        // 每个binding只能绑定一种输入数据的速率
        auto vertex_attr = factory->create_vertex_attribute_set({
            {
                .binding = 0,
                .rate = Match::InputRate::ePerVertex,  // 数据输入速率（每个顶点输入一份）
                //                  pos是三个float                      color是三个float              uv是两个float
                .attributes = { Match::VertexType::eFloat3, Match::VertexType::eFloat3, Match::VertexType::eFloat2 }
            },
            {
                .binding = 1,
                .rate = Match::InputRate::ePerInstance,  // 每个实例输入一份offset数据
                .attributes = { Match::VertexType::eFloat3 }  // offset是三个float
            }
        });

        // 创建DescriptorSet
        auto descriptor_set = factory->create_descriptor_set(renderer);
        // 添加所有Shader的描述符
        descriptor_set->add_descriptors({
            { Match::ShaderStage::eVertex, 0, Match::DescriptorType::eUniform },
            { Match::ShaderStage::eVertex, 1, Match::DescriptorType::eUniform },
            // 可以设置Descriptor的数量，默认为1
            { Match::ShaderStage::eFragment, 2, Match::DescriptorType::eTexture, 1 }
        }).allocate();
        // 分配DescriptorSet，如果不显式分配的话，ShaderProgram在调用compile方法时会自动分配，只有分配过的DescriptorSet才能绑定资源到描述符

        // 资源描述符的绑定由DescriptorSet完成，因为资源描述符与Shader文件有关并且能被ShaderProgram共用，与ShaderProgram无关
        // 不再从shader处添加描述符，descriptor_set应该是能被多个ShaderProgram共用的，所以分离出来

        // 创建PushConstants
        auto push_constants = factory->create_push_constants(
            // 指定该Constants被使用的ShaderStage
            Match::ShaderStage::eVertex,
            // 配置Constants的内容
            {
                { "pad", Match::ConstantType::eFloat },
                { "t", Match::ConstantType::eFloat },
        });

        // 为renderer创建shader_program
        auto shader_program = factory->create_shader_program(renderer, "MainSubpass");
        // 绑定顶点数据描述符
        // ShaderProgram在compile前的配置方法统一用attach前缀
        shader_program->attach_vertex_attribute_set(vertex_attr);
        shader_program->attach_vertex_shader(vert_shader, "main");
        shader_program->attach_fragment_shader(frag_shader, "main");
        // 添加DescriptorSet
        shader_program->attach_descriptor_set(descriptor_set);
        // 可自定义每个DescriptorSet绑定到第几个Set上，默认第0个
        // shader_program->attach_descriptor_set(descriptor_set, 2);
        // compile时会自动检查DescriptorSet有没有分配过，若没有，自动分配
        // 绑定PushConstants，不同于DescriptorSet，一个ShaderProgram只能绑定一个PushConstants
        shader_program->attach_push_constants(push_constants);
        shader_program->compile({
            .cull_mode = Match::CullMode::eNone,  // 取消面剔除
            .depth_test_enable = VK_TRUE,         // 启用深度测试
            .dynamic_states = {
                vk::DynamicState::eViewport,    // 动态Viewport
                vk::DynamicState::eScissor,     // 动态Scissor
            }
        });

        // 为每个binding创建VertexBuffer
        auto vert_buffer_0 = factory->create_vertex_buffer(sizeof(Vertex), 1024);
        auto vert_buffer_1 = factory->create_vertex_buffer(sizeof(glm::vec3), 1024);
        // 创建IndexBuffer
        auto index_buffer = factory->create_index_buffer(Match::IndexType::eUint16, 1024);
        // 映射到内存
        void *vertex_ptr_0 = vert_buffer_0->map();
        void *vertex_ptr_1 = vert_buffer_1->map();
        uint16_t *index_ptr = (uint16_t *)index_buffer->map();
        // 将顶点数据写回顶点缓存(VertexBuffer)
        memcpy(vertex_ptr_0, vertices.data(), sizeof(Vertex) * vertices.size());
        memcpy(vertex_ptr_1, offsets.data(), sizeof(glm::vec3) * offsets.size());
        // 将内存数据刷新到显存中
        memcpy(index_ptr, indices.data(), indices.size() * sizeof(uint16_t));
        // unmap时改为不自动flush，减少性能损耗
        vert_buffer_0->flush();
        vert_buffer_1->flush();
        index_buffer->flush();
        // 记得unmap顶点缓存（不unmap也行，buffer在析构时会自动unmap）
        vert_buffer_0->unmap();
        vert_buffer_1->unmap();
        index_buffer->unmap();

        // 不再可以自动获取资源描述符对应的资源
        // 需要手动创建资源并绑定到对应的描述符上
        auto pos_uniform = factory->create_uniform_buffer(sizeof(PosScaler));
        auto color_uniform = factory->create_uniform_buffer(sizeof(ColorScaler));

        // 创建纹理，
        // auto texture = factory->load_texture("moon.jpg", 4);  // 为Texture生成4层mipmap
        // 支持KTX Texture
        auto texture = factory->load_texture("moon.ktx");
        MCH_INFO("KTX Texture Mip Levels: {}", texture->get_mip_levels());

        // 创建采样器（可配置采样器选项）
        auto sampler = factory->create_sampler({
            .min_filter = Match::SamplerFilter::eNearest,
            .address_mode_u = Match::SamplerAddressMode::eMirroredRepeat,
            .address_mode_v = Match::SamplerAddressMode::eClampToBorder,
            // 设置各向异性过滤
            .max_anisotropy = 16,
            .border_color = Match::SamplerBorderColor::eFloatOpaqueWhite,
            .mip_levels = texture->get_mip_levels(),
        });

        // 将创建的资源绑定到对应的binding
        // 已分配的DescriptorSet才能绑定资源
        // 可以在一个binding上绑定到多个数量的descriptor
        // 对一默认的1个descriptor也有简化的方法
        descriptor_set->bind_uniform(0, pos_uniform);
        // descriptor_set->bind_uniforms(0, { pos_uniform });
        descriptor_set->bind_uniform(1, color_uniform);
        // descriptor_set->bind_uniforms(1, { color_uniform });
        descriptor_set->bind_texture(2, texture, sampler);
        // descriptor_set->bind_textures(2, { { texture, sampler } });

        // 或者改变深度缓存默认的深度
        // Vulkan支持的最大深度值在0到1之间，所以行不通
        // renderer->set_clear_value("Depth Buffer", { .depthStencil = { 5.0f, 0 } });

        while (!glfwWindowShouldClose(Match::window->get_glfw_window())) {
            glfwPollEvents();

            // 动态变换的三角形
            // 起始时间
            static auto start_time = std::chrono::high_resolution_clock::now();
            // 当前时间
            auto current_time = std::chrono::high_resolution_clock::now();
            // 时间差
            auto time = std::chrono::duration<float, std::chrono::seconds::period>(current_time-start_time).count();

            // 设置constants的值
            // 由PushConstants设置值，多个绑定着同一个PushConstants的ShaderProgram可以共享PushConstants的值
            push_constants->push_constant("t", time);

            // 程序启动5秒后关闭垂直同步，关闭后帧率涨到3000FPS
            static bool flag = true;
            if (time > 5 && flag) {
                flag = false;
                Match::runtime_setting->set_vsync(false);
                renderer->update_resources();
            }
            // cos(时间差)
            float scale = std::cos(time * 3);  // 加速3倍

            // 动态变换的背景颜色
            float color = (scale + 3) / 4;
            renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { 0.3f, 1 - color, color, 1.0f } });

            // 将数据写入UniformBuffer
            auto pos_scaler = (PosScaler *)pos_uniform->get_uniform_ptr();
            auto color_scaler = (ColorScaler *)color_uniform->get_uniform_ptr();
            // 将scale范围从[-1, 1]变换到[0.5, 1.5]
            pos_scaler->x_pos_scale = (scale + 2) / 2;
            // 将scale范围从[-1, 1]变换到[0.3, 1.0]
            // pos_scaler->y_pos_scale = (scale + 2) / 3;
            pos_scaler->y_pos_scale = pos_scaler->x_pos_scale;
            // 将scale范围从[-1, 1]变换到[0.6, 1.3]
            color_scaler->color_scale = (scale + 3) / 3;

            renderer->begin_render();
            renderer->bind_shader_program(shader_program);
            // 动态设置Viewport和Scissor
            auto &size = Match::runtime_setting->get_window_size();
            renderer->set_viewport(0, static_cast<float>(size.height), static_cast<float>(size.width), -static_cast<float>(size.height));
            renderer->set_scissor(0, 0, size.width, size.height);
            // 绑定每个binding的VertexBuffer
            // 参数中的VertexBuffer将根据他们在参数中的位置自动绑定到对应的binding
            renderer->bind_vertex_buffers({
                vert_buffer_0, // 第一个绑定到binding 0
                vert_buffer_1, // 第二个绑定到binding 1
            });
            // 绑定IndexBuffer
            renderer->bind_index_buffer(index_buffer);
            // DrawCall
            // renderer.draw(vertices.size(), 1, 0, 0);
            // IndexDrawCall
            // 实例渲染, 共渲染4个实例
            renderer->draw_indexed(indices.size(), offsets.size(), 0, 0, 0);

            // 进行ImGui的渲染，最后进行
            renderer->begin_layer_render("ImGui");
            static bool show = true;
            if (show) {
                ImGui::ShowDemoWindow(&show);
            }
            ImGui::Text("测试中文字体");
            renderer->end_layer_render("ImGui");

            renderer->end_render();
        }
        // 等待GPU处理完所有数据，再销毁资源，否则会销毁GPU正在使用的资源导致报错
        renderer->wait_for_destroy();
    } // 离开作用域后所有创建的资源会被销毁

    // 销毁Match
    Match::Destroy();
    return 0;
}
