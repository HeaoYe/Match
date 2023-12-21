#include <Match/Match.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(800, 800, "window", nullptr, nullptr);
    glfwSetWindowPos(window, 100, 50);

    // 配置Match
    Match::setting.debug_mode = true;
    Match::setting.device_name = Match::AUTO_SELECT_DEVICE;
    Match::setting.render_backend = Match::PlatformWindowSystem::eXlib;
    Match::setting.app_name = "App Name";

    // 配置VulkanAPI初始化信息，用于生成VkSurfaceKHR
    auto &api_info = Match::CreateAPIInfo();
    // Match::setting.render_backend = Match::PlatformWindowSystem::eXlib;
    // 上面选择了Xlib，这里就要给Vulkan Xlib的信息
    api_info.xlib_display = glfwGetX11Display();
    api_info.xlib_window = glfwGetX11Window(window);

    // 初始化Match
    auto &context = Match::Initialize();
    // 显示全部设备名，可填写在Match::setting.device_name中
    // Match::EnumerateDevices();

    {
        // 初始化资源工厂，会在${root}/shaders文件夹下寻找Shader文件，，会在${root}/textures文件夹下寻找图片文件
        auto factory = context.create_resource_factory("./resource");
        
        // 配置Vulkan的RenderPass
        auto builder = factory->create_render_pass_builder();
        builder->add_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, Match::AttchmentType::eColor);
        // builder.add_custom_attachment("自定义名字", {
            // 填写VkAttachmentDescription结构体
        // })
        // 设置最总要呈现到屏幕的Attachment
        builder->set_final_present_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);

        // 创建Subpass
        auto &subpass = builder->create_subpass("MainSubpass");
        // 绑定Subpass为图形流水线
        subpass.bind(VK_PIPELINE_BIND_POINT_GRAPHICS);
        // 设置Subpass的输出为SWAPCHAIN_IMAGE_ATTACHMENT
        subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        // 添加Subpass Dependency
        subpass.wait_for(
            Match::EXTERNAL_SUBPASS,
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
            { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_NONE }
        );
        // 将RenderPass与Renderer绑定
        auto renderer = context.create_renderer(builder);

        // 加载已编译的Shader文件
        // auto vert_shader = factory->load_shader("vert.spv");
        // auto frag_shader = factory->load_shader("frag.spv");
        // 动态编译Shader文件
        auto vert_shader = factory->load_shader("shader.vert", Match::ShaderType::eVertexShaderNeedCompile);
        auto frag_shader = factory->load_shader("shader.frag", Match::ShaderType::eFragmentShaderNeedCompile);
        
        // 创建顶点数据结构体
        struct Vertex {
            glm::vec2 pos;
            glm::vec3 color;
            glm::vec2 uv;
        };
        // 顶点数据
        const std::vector<Vertex> vertices = {
            { { 0.5f, 0.5f }, { 0.5f, 0.2f, 0.2f }, { 1.0f, 1.0f } },
            { { 0.5f, -0.5f }, { 0.2f, 0.5f, 0.2f }, { 1.0f, 0.0f } },
            { { -0.5f, 0.5f }, { 0.2f, 0.5f, 0.2f }, { 0.0f, 1.0f } },
            { { -0.5f, -0.5f }, { 0.2f, 0.2f, 0.5f }, { 0.0f, 0.0f } },
        };
        // 每个实例的offset
        // 渲染顺序与offsets的顺序有关
        const std::vector<glm::vec2> offsets = {
            { 0.5, 0.5 },   // 第一象限
            { -0.5, 0.5 },  // 第二象限  // 覆盖第一象限
            { 0.5, -0.5 },  // 第四象限
            { -0.5, -0.5 }, // 第三象限  // 覆盖第四象限
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
                //                  pos是两个float                      color是三个float              uv是两个float
                .attributes = { Match::VertexType::eFloat2, Match::VertexType::eFloat3, Match::VertexType::eFloat2 }
            },
            {
                .binding = 1,
                .rate = Match::InputRate::ePerInstance,  // 每个实例输入一份offset数据
                .attributes = { Match::VertexType::eFloat2 }  // offset是两个float
            }
        });

        // 资源描述符的绑定由Shader完成，因为资源描述符与Shader文件有关，与ShaderProgram无关
        // 为vertex shader添加Uniform描述符
        vert_shader->bind_descriptors({
            // { binding, descriptor type, sizeof uniform }
            { 0, Match::DescriptorType::eUniform, sizeof(PosScaler) },
            { 1, Match::DescriptorType::eUniform, sizeof(ColorScaler) },
        });
        // 为fragment shader添加Texture描述符
        frag_shader->bind_descriptors({
            { 2, Match::DescriptorType::eTexture }
        });
        
        // 为renderer创建shader_program
        auto shader_program = factory->create_shader_program(renderer, "MainSubpass");
        // 绑定顶点数据描述符
        shader_program->bind_vertex_attribute_set(vertex_attr);
        shader_program->attach_vertex_shader(vert_shader, "main");
        shader_program->attach_fragment_shader(frag_shader, "main");
        shader_program->compile({
            .cull_mode = Match::CullMode::eNone,  // 取消面剔除
        });

        // 为每个binding创建VertexBuffer
        auto vert_buffer_0 = factory->create_vertex_buffer(sizeof(Vertex), 1024);
        auto vert_buffer_1 = factory->create_vertex_buffer(sizeof(glm::vec2), 1024);
        // 创建IndexBuffer
        auto index_buffer = factory->create_index_buffer(Match::IndexType::eUint16, 1024);
        // 映射到内存
        void *vertex_ptr_0 = vert_buffer_0->map();
        void *vertex_ptr_1 = vert_buffer_1->map();
        uint16_t *index_ptr = (uint16_t *)index_buffer->map();
        // 将顶点数据写回顶点缓存(VertexBuffer)
        memcpy(vertex_ptr_0, vertices.data(), sizeof(Vertex) * vertices.size());
        memcpy(vertex_ptr_1, offsets.data(), sizeof(glm::vec2) * offsets.size());
        // 将内存数据刷新到显存中
        memcpy(index_ptr, indices.data(), indices.size() * 2);
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

        // 创建采样器（可配置采样器选项）
        auto sampler = factory->create_sampler({
            .min_filter = Match::SamplerFilter::eNearest
        });
        // 创建纹理，
        auto texture = factory->create_texture("moon.jpg");

        // 将创建的资源绑定到对应的binding
        shader_program->bind_uniforms(0, { pos_uniform });
        shader_program->bind_uniforms(1, { color_uniform });
        shader_program->bind_textures(2, { texture }, { sampler });

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // 动态变换的三角形
            
            // 起始时间
            static auto start_time = std::chrono::high_resolution_clock::now();
            // 当前时间
            auto current_time = std::chrono::high_resolution_clock::now();
            // cos(时间差)
            float scale = std::cos(std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count() * 3);  // 加速3倍

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
            renderer->draw_indexed(6, 4, 0, 0, 0);
            renderer->end_render();
        }
        // 等待GPU处理完所有数据，再销毁资源，否则会销毁GPU正在使用的资源导致报错
        renderer->wait_for_destroy();
    } // 离开作用域后所有创建的资源会被销毁

    // 销毁Match
    Match::Destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
