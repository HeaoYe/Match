project(ImGUI)

file(GLOB source CONFIGURE_DEPENDS imgui/*.cpp imgui/backends/imgui_impl_vulkan.cpp)
# file(GLOB source CONFIGURE_DEPENDS imgui/backends/imgui_impl_glfw.cpp)

add_library(ImGUI SHARED ${source})
target_include_directories(ImGUI PUBLIC imgui)
