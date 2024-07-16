project(ImGui)

file(GLOB source CONFIGURE_DEPENDS imgui/*.cpp imgui/backends/imgui_impl_vulkan.cpp imgui/backends/imgui_impl_glfw.cpp)

add_library(ImGui SHARED ${source})
target_include_directories(ImGui PUBLIC imgui)

if (WIN32)
    target_compile_definitions(ImGui PRIVATE IMGUI_API=__declspec\(dllexport\))
endif ()

# link glfw
target_link_libraries(ImGui PUBLIC glfw)
target_include_directories(ImGui PUBLIC "${GLFW_SOURCE_DIR}/deps")
target_link_libraries(ImGui PUBLIC Vulkan::Vulkan)
