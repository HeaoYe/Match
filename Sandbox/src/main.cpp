#include <Match/Match.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>
#include <glm/glm.hpp>

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(400, 400, "window", nullptr, nullptr);
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

    // 配置Vulkan的RenderPass

    auto &builder = context.get_render_pass_builder();
    builder.add_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT, Match::AttchmentType::eColor);
    // builder.add_custom_attachment("自定义名字", {
        // 填写VkAttachmentDescription结构体
    // })
    // 设置最总要呈现到屏幕的Attachment
    builder.set_final_present_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);

    // 创建Subpass
    auto &subpass = builder.create_subpass("MainSubpass");
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
    // 创建RenderPass
    context.build_render_pass();

    {
        // 初始化资源工厂，会在${root}/shaders文件夹下寻找Shader文件
        auto factory = context.create_resource_factory("./resource");
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
        };
        // 顶点数据
        std::vector<Vertex> vertices = {
            { { 0.0f, 0.5f }, { 0.5f, 0.2f, 0.2f } },
            { { 0.5f, -0.5f }, { 0.2f, 0.5f, 0.2f } },
            { { -0.5f, -0.5f }, { 0.2f, 0.2f, 0.5f } }
        };

        // 创建顶点数据描述符
        auto vertex_attr = factory->create_vertex_attribute();
        vertex_attr->add_input_attribute(Match::VertexType::eFloat2);  // pos是两个float
        vertex_attr->add_input_attribute(Match::VertexType::eFloat3);  // color是三个float
        vertex_attr->add_input_binding(Match::InputRate::ePerVertex);  // 数据输入速率（每个顶点输入一份）

        auto shader_program = factory->create_shader_program("MainSubpass");
        // 绑定顶点数据描述符
        shader_program->bind_vertex_attribute_set(vertex_attr);
        shader_program->attach_vertex_shader(vert_shader, "main");
        shader_program->attach_fragment_shader(frag_shader, "main");
        shader_program->compile({
            .cull_mode = Match::CullMode::eNone,  // 取消面剔除
        });

        // 创建VertexBuffer
        auto vert_buffer = factory->create_vertex_buffer(sizeof(Vertex), 1024);
        // 映射到内存
        void *data_ptr = vert_buffer->map();

        Match::Renderer renderer(2);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            // 动态变换的三角形
            static float scale = 0.99;
            if (vertices[0].pos.y < 0.05) {
                scale = 1.01;
            } else if (vertices[0].pos.y > 0.85) {
                scale = 0.99;
            }
            vertices[0].pos *= scale;
            vertices[1].pos *= scale;
            vertices[2].pos *= scale;
            vertices[0].color.r *= scale;
            vertices[1].color.g *= scale;
            vertices[2].color.b *= scale;
            // 将顶点数据写回顶点缓存(VertexBuffer)
            memcpy(data_ptr, vertices.data(), sizeof(Vertex) * vertices.size());
            // 将内存数据刷新到显存中
            vert_buffer->flush();

            renderer.begin_render();
            renderer.bind_shader_program(shader_program);
            // 绑定VertexBuffer
            renderer.bind_vertex_buffers({ vert_buffer });
            // DrawCall
            renderer.draw(vertices.size(), 1, 0, 0);
            renderer.end_render();
        }
        vert_buffer->unmap();  // 记得unmap顶点缓存（不unmap也行）
    } // 离开作用域后所有创建的资源会被销毁

    // 销毁Match
    Match::Destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
