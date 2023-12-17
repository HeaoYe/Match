#include <Match/Match.hpp>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

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
        auto vert_shader = factory->load_shader("vert.spv");
        auto frag_shader = factory->load_shader("frag.spv");
        auto shader_program = factory->create_shader_program("MainSubpass");
        shader_program->attach_vertex_shader(vert_shader, "main");
        shader_program->attach_fragment_shader(frag_shader, "main");
        shader_program->compile({
            .cull_mode = Match::CullMode::eNone,  // 取消面剔除
        });

        auto pool = context.create_command_pool();
        Match::Renderer renderer(*pool, 2);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            renderer.begin_render();
            renderer.bind_shader_program(shader_program);
            VkCommandBuffer buffer = renderer.get_command_buffer();
            vkCmdDraw(buffer, 3, 1, 0, 0);
            renderer.end_render();
        }
    } // 离开作用域后所有创建的资源会被销毁

    // 销毁Match
    Match::Destroy();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
