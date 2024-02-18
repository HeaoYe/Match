# Match
A cross platform vulkan renderer


# Build

## Windows

### 1.安装VSCode，不演示了，网上有很多教学

### 2.安装Python，不演示了，网上有很多教学
    Win+r 输入cmd
    然后输入python --version
    如果有输出说明python安装成功
    建议python版本 >= 3.7

### 3.安装VSCode插件
    <1> Gruvbox Theme（可选）
    <2> Markdown Preview（可选）
    <3> CMake
    <4> CMake Tools
    <5> CodeLLDB
    <6> clangd

### 4.安装CMake
    https://cmake.org/
    点击Download
    下载Windows x64 ZIP	cmake-3.29.0-rc1-windows-x86_64.zip
    解压到一个文件夹中并记住

### 5.安装MSYS2
    https://www.msys2.org/
    Download the installer: msys2-x86_64-20240113.exe
    一直下一步就行，记得设置安装位置并记下来
    安装好后点击 立即运行MSYS2
    输入命令
    pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain
    一直按回车就行
    网不好的多重试几次
    安装好后输入命令验证是否安装成功
    gcc --version
    g++ --version

### 6.安装Git
    https://git-scm.com/
    点击Downloads
    点击Windows
    点击64-bit Git for Windows Setup.
    一直Next就行，记得设置安装位置并记下来
    也可以根据自己的需求进行配置

### 7.安装VulkanSDK
    https://vulkan.lunarg.com/
    点击SDK
    下载并运行 VulkanSDK-1.3.275.0-Installer.exe (160MB)
    设置安装位置并记下来
    根据需求选择要安装的组件
    网不好的多重试几次

### 8.设置环境变量
    设置 -> 系统 -> 关于 -> 高级系统设置 -> 环境变量
    在Path中添加CMake的目录和MSYS2的目录和VulkanSDK的目录
    CMake:
    CMake解压目录\cmake-3.29.0-rc1-windows-x86_64\bin
    MSYS2:
    MSYS64安装目录\ucrt64\bin
    VulkanSDK:
    VulkanSDK安装目录\Bin
    点击新建 -> 输入路径 -> 点击上移 移动到最上面
    设置好后，Win+r 输入cmd
    输入命令验证安装是否成功
    cmake --version
    gcc --version
    g++ --version
    vkcube

### 9.克隆Match
    mkdir NewProject
    cd NewProject

    git clone https://github.com/HeaoYe/Match
    cd Match
    git submodule update --init --recursive
    cd thirdparty/shaderc
    python ./utils/git-sync-deps

### 10.构建并运行Match
    新建CMakeLists.txt
    只有Windows需要设置BASH_EXECUTABLE为GitBash的可执行文件目录
    set(BASH_EXECUTABLE Git安装目录\\bin\\bash.exe)
    add_subdirectory(Match)

    F7构建项目，如果没反应的话重启VSCode
    在cpp文件中按 F5运行
    配置.vscode/launch.json中的运行信息

    因为Windows的DLL需要和可执行文件放在一起
    所以examples中的每个项目都会复制DLL到对应的build目录
    很费空间
    可以使用set(MATCH_BUILD_EXAMPLES OFF)禁止构建examples

    C++绑定的VulkanAPI在需要重建交换链时会抛异常，VSCode会在抛异常时自动断点调试
    取消勾选C++: on throw就行了

### 11.使用
    target_link_libraries(TARGET Match)
    // Windows用户需要加入COPYDLL来复制所需的DLL文件
    COPYDLL(TARGET .)
