# Add Shared Library
file(GLOB_RECURSE source CONFIGURE_DEPENDS src/Match/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/Match/*.hpp)

add_library(Match SHARED ${source} ${headers})
target_include_directories(Match PUBLIC include)

target_link_libraries(Match
    PUBLIC
        glm::glm-header-only
        ImGui
        tinygltf
        spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>
        Vulkan::Vulkan
        VulkanMemoryAllocator
    PRIVATE
        tinyobjloader
        ktx
        glslang glslang::SPIRV glslang::glslang-default-resource-limits
)
target_include_directories(Match PRIVATE ../thirdparty/stb)

target_compile_definitions(Match PRIVATE MATCH_INNER_VISIBLE)
if (WIN32)
    target_compile_definitions(Match PUBLIC PLATFORM_WINDOWS)
    target_compile_definitions(Match PRIVATE WINDOWS_DLL)
    target_compile_definitions(Match PUBLIC IMGUI_API=__declspec\(dllimport\))
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Match PRIVATE -O3)
endif()

function(COPYDLL TARGET_NAME PATH_PREFIX)
    add_custom_command(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PATH_PREFIX}"/Match/Match.dll"
        ${PATH_PREFIX}"/thirdparty/imgui/imgui.dll"
        ${PATH_PREFIX}"/thirdparty/glfw/src/glfw3.dll"
        $<TARGET_FILE_DIR:${TARGET_NAME}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endfunction(COPYDLL)
