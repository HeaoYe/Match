project(Match)

# Set Window System
option(USE_WAYLAND "Use Wayland" ON)
option(USE_XCB "Use XCB" ON)
option(USE_XLIB "Use XLIB" ON)
option(USE_WIN32 "Use WIN32" OFF)

# Add Shared Library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
file(GLOB_RECURSE source CONFIGURE_DEPENDS src/Match/*.cpp)
add_library(Match SHARED ${source})

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
target_include_directories(Match PUBLIC include)

# Link Render API
target_link_libraries(Match PUBLIC Vulkan::Vulkan)
target_link_libraries(Match PUBLIC VulkanMemoryAllocator)

# Link Shaderc
target_link_libraries(Match PRIVATE shaderc_shared)

target_compile_definitions(Match PRIVATE MATCH_INNER_VISIBLE)
