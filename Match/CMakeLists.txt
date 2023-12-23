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

# link glfw
target_link_libraries(Match PUBLIC glfw)
target_include_directories(Match PUBLIC "${GLFW_SOURCE_DIR}/deps")

# Link imgui
target_link_libraries(Match PUBLIC ImGUI)

# Link stb
target_include_directories(Match PRIVATE ../${stb_DIR})

# Link spdlog
target_link_libraries(Match PUBLIC spdlog::spdlog $<$<BOOL:${MINGW}>:ws2_32>)
target_include_directories(Match PUBLIC include)

# Link Render API
target_link_libraries(Match PUBLIC Vulkan::Vulkan)
target_link_libraries(Match PUBLIC shaderc)
target_link_libraries(Match PUBLIC VulkanMemoryAllocator)

# Link Window System
if(USE_WAYLAND)
    target_compile_definitions(Match PUBLIC MATCH_WAYLAND)
endif()
if(USE_XCB)
    target_compile_definitions(Match PUBLIC MATCH_XCB)
endif()
if(USE_XLIB)
    target_compile_definitions(Match PUBLIC MATCH_XLIB)
endif()
if(USE_WIN32)
    target_compile_definitions(Match PUBLIC MATCH_WIN32)
endif()

target_compile_definitions(Match PRIVATE MATCH_INNER_VISIBLE)
