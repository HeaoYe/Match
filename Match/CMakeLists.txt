project(Match)

# Add Shared Library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
file(GLOB_RECURSE source CONFIGURE_DEPENDS src/Match/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS include/Match/*.hpp)
add_library(Match SHARED ${source} ${headers})
target_include_directories(Match PUBLIC include)

# Link tinygltf
target_link_libraries(Match PUBLIC tinygltf)

# Link tinyobjloader
target_link_libraries(Match PRIVATE tinyobjloader)

# Link glm
target_link_libraries(Match PUBLIC glm::glm)

# Link KTX-Software
target_link_libraries(Match PRIVATE ktx)

# Link imgui
target_link_libraries(Match PUBLIC ImGui)

# Link stb
target_include_directories(Match PRIVATE ../${stb_DIR})

# Link spdlog
target_link_libraries(Match PUBLIC spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)

# Link Render API
target_link_libraries(Match PUBLIC Vulkan::Vulkan)
target_link_libraries(Match PUBLIC VulkanMemoryAllocator)

# Link Shaderc
target_link_libraries(Match PRIVATE shaderc_shared)

target_compile_definitions(Match PRIVATE MATCH_INNER_VISIBLE)
if (WIN32)
    target_compile_definitions(Match PUBLIC PLATFORM_WINDOWS)
    target_compile_definitions(Match PRIVATE WINDOWS_DLL)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(Match PRIVATE -O3)
endif()

function(COPYDLL TARGET_NAME PATH_PREFIX)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PATH_PREFIX}"/Match/libMatch.dll"
            ${PATH_PREFIX}"/thirdparty/glfw/src/glfw3.dll"
            ${PATH_PREFIX}"/thirdparty/imgui/libImGui.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/libshaderc/libshaderc_shared.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/third_party/glslang/SPIRV/SPIRVd.dll"
            ${PATH_PREFIX}"/thirdparty/spdlog/libspdlogd.dll"
            ${PATH_PREFIX}"/thirdparty/tinygltf/libtinygltf.dll"
            ${PATH_PREFIX}"/thirdparty/tinyobjloader/libtinyobjloader.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PATH_PREFIX}"/Match/libMatch.dll"
            ${PATH_PREFIX}"/thirdparty/glfw/src/glfw3.dll"
            ${PATH_PREFIX}"/thirdparty/imgui/libImGui.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/libshaderc/libshaderc_shared.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/third_party/glslang/SPIRV/SPIRV.dll"
            ${PATH_PREFIX}"/thirdparty/spdlog/libspdlog.dll"
            ${PATH_PREFIX}"/thirdparty/tinygltf/libtinygltf.dll"
            ${PATH_PREFIX}"/thirdparty/tinyobjloader/libtinyobjloader.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PATH_PREFIX}"/Match/libMatch.dll"
            ${PATH_PREFIX}"/thirdparty/glfw/src/glfw3.dll"
            ${PATH_PREFIX}"/thirdparty/imgui/libImGui.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/libshaderc/libshaderc_shared.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/third_party/glslang/SPIRV/SPIRV.dll"
            ${PATH_PREFIX}"/thirdparty/spdlog/libspdlog.dll"
            ${PATH_PREFIX}"/thirdparty/tinygltf/libtinygltf.dll"
            ${PATH_PREFIX}"/thirdparty/tinyobjloader/libtinyobjloader.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_custom_command(
            TARGET ${TARGET_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${PATH_PREFIX}"/Match/libMatch.dll"
            ${PATH_PREFIX}"/thirdparty/glfw/src/glfw3.dll"
            ${PATH_PREFIX}"/thirdparty/imgui/libImGui.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/libshaderc/libshaderc_shared.dll"
            ${PATH_PREFIX}"/thirdparty/shaderc/third_party/glslang/SPIRV/SPIRV.dll"
            ${PATH_PREFIX}"/thirdparty/spdlog/libspdlog.dll"
            ${PATH_PREFIX}"/thirdparty/tinygltf/libtinygltf.dll"
            ${PATH_PREFIX}"/thirdparty/tinyobjloader/libtinyobjloader.dll"
            $<TARGET_FILE_DIR:${TARGET_NAME}>
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        )
    endif ()
endfunction(COPYDLL)
