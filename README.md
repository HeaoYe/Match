# Match
一个跨平台的Vulkan渲染引擎

A cross platform vulkan renderer

# 一、环境搭建
- [Windows](#在Windows平台搭建环境)
- [Linux](#在Linux平台搭建环境)

# 二、克隆并初始化仓库
```bash
git clone https://github.com/HeaoYe/Match
cd Match
git submodule update --init --recursive
cd thirdparty/shaderc
python ./utils/git-sync-deps
```

# 三、编译与运行
1. 用VSCode打开Match
2. F7编译
3. 点击左侧Run And Debug按钮
4. 选择要运行的程序
5. F5运行
6. VSCode默认会在程序抛出异常时中断
    - 但是Vulkan的C++API在需要重建交换链时也会抛异常
    - 导致窗口大小改变时，程序会中断
    - 取消勾选 **C++: on throw** 即可

# 四、将Match作为第三方库
1. 新建MyProject文件夹并进入
```bash
mkdir MyProject
cd MyProject
```
2. 克隆Match并根据[上述步骤](#二克隆并初始化仓库)初始化仓库
3. 在MyProject文件夹下，新建CMakeLists.txt
```cmake
cmake_minimum_required(VERSION 3.27)

project(MyProject)

# 不构建案例代码
set(MATCH_BUILD_EXAMPLES OFF)

if (WIN32)
    # set(BASH_EXECUTABLE "你安装的Git中的bash.exe的完整路径")
    # 如果你把xxx\Git\bin加入了环境变量，可以省略这步
    # * 注意！如果你的Windows启用了Linux子系统功能
    # * 那么C:\Windows\System32目录下会有bash.exe文件
    # * 这种情况下无论如何都不能省略这步
    set(BASH_EXECUTABLE "D:\\Git\\bin\\bash.exe")
endif()

add_subdirectory(Match)

add_executable(MyProject main.cpp)
target_link_libraries(MyProject Match)
if (WIN32)
    # COPYDLL(TARGET_NAME 在当前CMakeLists.txt的目录下,与Match仓库的相对路径)
    COPYDLL(MyProject ./Match)
endif()
```
4. 在MyProject文件夹下，新建main.cpp

#### 最简示例
```cpp
#include <Match/Match.hpp>

int main() {
    // +++ 初始化Match
    auto &ctx = Match::Initialize();

    // +++ GameLoop
    while (Match::window->is_alive()) {
        // +++ 处理事件
        Match::window->poll_events();
    }

    // +++ 销毁Match
    Match::Destroy();
    return 0;
}
```

#### 设置背景颜色 + 启用8xMSAA抗锯齿
```cpp
#include <Match/Match.hpp>

int main() {
    // 初始化Match
    auto &ctx = Match::Initialize();

    {
        // +++ 创建资源工厂
        auto factory = ctx.create_resource_factory("");

        // +++ 启用8xMSAA抗锯齿
        Match::runtime_setting->set_multisample_count(Match::SampleCount::e8);

        // +++ 创建RenderPass
        auto render_pass_builder = factory->create_render_pass_builder();
        auto &main_subpass = render_pass_builder->add_subpass("Main Subpass");
        main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        
        // +++ 创建Renderer
        auto renderer = factory->create_renderer(render_pass_builder);
        
        // +++ 设置背景颜色
        renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { 0.3f, 0.7f, 0.4f, 1.0f } });

        // GameLoop
        while (Match::window->is_alive()) {
            // 处理事件
            Match::window->poll_events();

            // +++ 开始渲染
            renderer->begin_render();
            // +++ 结束渲染
            renderer->end_render();
        }

        // +++ 等待销毁
        renderer->wait_for_destroy();
    }

    // 销毁Match
    Match::Destroy();
    return 0;
}
```

#### 添加ImGui
```cpp
#include <Match/Match.hpp>
// +++ 
#include <imgui.h>

int main() {
    // +++ 设置字体大小
    Match::setting.font_size = 30.0f;

    // 初始化Match
    auto &ctx = Match::Initialize();

    {
        // 创建资源工厂
        auto factory = ctx.create_resource_factory("");

        // 启用8xMSAA抗锯齿
        Match::runtime_setting->set_multisample_count(Match::SampleCount::e8);

        // 创建RenderPass
        auto render_pass_builder = factory->create_render_pass_builder();
        auto &main_subpass = render_pass_builder->add_subpass("Main Subpass");
        main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        
        // 创建Renderer
        auto renderer = factory->create_renderer(render_pass_builder);

        // +++ 附加ImGui渲染层
        renderer->attach_render_layer<Match::ImGuiLayer>("My ImGui Layer");
        
        // 设置背景颜色
        renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { 0.3f, 0.7f, 0.4f, 1.0f } });

        // GameLoop
        while (Match::window->is_alive()) {
            // 处理事件
            Match::window->poll_events();

            // 开始渲染
            renderer->begin_render();

            // +++ 开始My ImGui Layer渲染
            renderer->begin_layer_render("My ImGui Layer");

            // 绘制ImGui
            ImGui::Begin("My First Frame");
            ImGui::Text("Hello World !");
            ImGui::End();

            // +++ 结束My ImGui Layer渲染
            renderer->end_layer_render("My ImGui Layer");

            // 结束渲染
            renderer->end_render();
        }

        // 等待销毁
        renderer->wait_for_destroy();
    }

    // 销毁Match
    Match::Destroy();
    return 0;
}
```

#### 绘制三角形
```cpp
#include <Match/Match.hpp>
#include <imgui.h>

int main() {
    // 设置字体大小
    Match::setting.font_size = 30.0f;

    // 初始化Match
    auto &ctx = Match::Initialize();

    {
        // 创建资源工厂
        auto factory = ctx.create_resource_factory("");
        
        // 启用8xMSAA抗锯齿
        Match::runtime_setting->set_multisample_count(Match::SampleCount::e8);

        // 创建RenderPass
        auto render_pass_builder = factory->create_render_pass_builder();
        auto &main_subpass = render_pass_builder->add_subpass("Main Subpass");
        main_subpass.attach_output_attachment(Match::SWAPCHAIN_IMAGE_ATTACHMENT);
        
        // 创建Renderer
        auto renderer = factory->create_renderer(render_pass_builder);
        
        // 附加ImGui渲染层
        renderer->attach_render_layer<Match::ImGuiLayer>("My ImGui Layer");
        
        // 设置背景颜色
        renderer->set_clear_value(Match::SWAPCHAIN_IMAGE_ATTACHMENT, { { 0.3f, 0.7f, 0.4f, 1.0f } });

        // +++ 从字符串编译 顶点着色器
        auto vert_shader = factory->compile_shader_from_string(
            "#version 450\n"
            "void main() {\n"
            "    vec2 pos = vec2(-0.8, -0.5);\n"
            "    if (gl_VertexIndex == 1) pos = vec2(0.8, -0.5);\n"
            "    if (gl_VertexIndex == 2) pos = vec2(0, 0.5);\n"
            "    gl_Position = vec4(pos, 0, 1);\n"
            "}",
            Match::ShaderStage::eVertex
        );
        // +++ 从字符串编译 片段着色器
        auto frag_shader = factory->compile_shader_from_string(
            "#version 450\n"
            "layout (location = 0) out vec4 out_color;\n"
            "void main() {\n"
            "    out_color = vec4(0.1, 0.3, 0.8, 1);\n"
            "}",
            Match::ShaderStage::eFragment
        );

        // +++ 创建着色器程序
        auto shader_program = factory->create_shader_program(renderer, "Main Subpass");
        // +++ 附加顶点着色器
        shader_program->attach_vertex_shader(vert_shader);
        // +++ 附加片段着色器
        shader_program->attach_fragment_shader(frag_shader);
        // +++ 编译着色器程序，禁用三角形剔除
        shader_program->compile({
            .cull_mode = Match::CullMode::eNone
        });

        // GameLoop
        while (Match::window->is_alive()) {
            // 处理事件
            Match::window->poll_events();

            // 开始渲染
            renderer->begin_render();
            
            // +++ 绑定着色器程序并绘制三角形
            renderer->bind_shader_program(shader_program);
            renderer->draw(3, 1, 0, 0);

            // 开始My ImGui Layer渲染
            renderer->begin_layer_render("My ImGui Layer");

            // 绘制ImGui
            ImGui::Begin("My First Frame");
            ImGui::Text("Hello World !");
            ImGui::End();

            // 结束My ImGui Layer渲染
            renderer->end_layer_render("My ImGui Layer");

            // 结束渲染
            renderer->end_render();
        }

        // 等待销毁
        renderer->wait_for_destroy();
    }

    // 销毁Match
    Match::Destroy();
    return 0;
}
```

#

## 在Windows平台搭建环境

### 1.安装Python，不演示了，网上有很多教学
1. Win+r 输入cmd
2. 在cmd输入
```bash
python --version
```
3. 如果输出python版本信息，说明python安装成功
4. 建议python版本 >= 3.7

### 2.安装CMake
[CMake官网](https://cmake.org/)
1. 点击Download
2. 下载Windows x64 ZIP	cmake-3.29.0-rc1-windows-x86_64.zip
3. 解压到一个文件夹中并记住

### 3.安装MSYS2
[MAYS2官网](https://www.msys2.org/)
1. Download the installer: msys2-x86_64-20240113.exe
2. 运行安装程序，一直下一步就行，记得设置安装位置并记下来
3. 安装好后点击 立即运行MSYS2
5. 输入命令，安装c++编译器，安装时一直按回车就行
```bash
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
```
5. 安装好后输入命令验证是否安装成功
```bash
gcc --version
g++ --version
```

### 4.安装Git
[Git官网](https://git-scm.com/)
1. 点击Downloads
2. 点击Windows
3. 点击64-bit Git for Windows Setup.
4. 运行安装程序，一直Next就行，记得设置安装位置并记下来
5. 也可以根据自己的需求进行配置

### 5.安装VulkanSDK
[VulkanSDK官网](https://vulkan.lunarg.com/)
1. 点击SDK
2. 下载并运行 VulkanSDK-1.3.275.0-Installer.exe (160MB)
3. 设置安装位置并记下来
4. 根据需求选择要安装的组件

### 6.设置环境变量
1. 设置 -> 系统 -> 关于 -> 高级系统设置 -> 环境变量
2. 需要在Path中添加 CMake的bin目录 & MSYS2的bin目录 & VulkanSDK的Bin目录
- CMake的bin目录:
例如 ```E:\CMake\cmake-3.29.0-rc1-windows-x86_64\bin```
- MSYS2的bin目录:
例如 ```E:\msys64\ucrt64\bin```
- VulkanSDK的Bin目录:
例如 ```E:\VulkanSDK\1.3.275.0\Bin```
3. 点击新建 -> 输入路径 -> 点击上移 移动到最上面
4. 设置好后，Win+r 输入cmd
5. 输入命令验证安装是否成功
```bash
cmake --version
gcc --version
g++ --version
vkcube
```

### 7.安装VSCode，不演示了，网上有很多教学

### 8.配置VSCode
[配置VSCode](#配置VSCode)

---

## 在Linux平台搭建环境

### 1.安装依赖
```bash
sudo pacman -S code vulkan-devel cmake clang git python 
```

### 2.配置VSCode
[配置VSCode](#配置VSCode)

---

## 配置VSCode
1. 安装插件
    - CMake
    - CMake Tools
    - CodeLLDB
    - clangd
    - Gruvbox Theme (可选)
    - Markdown Preview (可选)
2. 配置插件
    1. 进入Settings界面
    2. 搜索clangd
    3. 找到Clangd:Arguments
    4. 点击Add Item 分别添加两项
    - --compile-commands-dir=build
    - --header-insertion=never
3. 如何编译
    - 按F7编译带有CMakeLists.txt的项目。
    - 如果按F7没反应，确认打开的文件夹中有CMakeLists.txt，然后重启VSCode
    - 第一次编译需要选择C++编译器，Windows用户选择从MSYS2下载的gcc，linux选择gcc或clang
4. 如何运行
    - 在项目的CPP文件或头文件中，按F5运行
    - 第一次运行会生成.vscode/launch.json
    - 按照需求填写就行
    ```json
    {
        "version": "0.2.0",
        "configurations": [
            {
                "type": "lldb",
                "request": "launch",
                "name": "Debug",
                "program": "${workspaceFolder}/可执行文件",
                "args": [],
                "cwd": "${workspaceFolder}/可执行文件运行目录"
            }
        ]
    }
    ```
