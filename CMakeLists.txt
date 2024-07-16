cmake_minimum_required(VERSION 3.27)

if (PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message("WARN: PROJECT_BINARY_DIR == PROJECT_SOURCE_DIR")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
if (NOT CMAKE_BUILD_TYPE)
    message("default set CMAKE_BUILD_TYPE to Release")
    set(CMAKE_BUILD_TYPE Release)
endif()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("DEBUG MODE")
endif()

if (WIN32)
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

if (NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message("Found CCache: ${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
    endif()
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
    Match
    LANGUAGES CXX C
    DESCRIPTION "一个跨平台的Vulkan渲染器"
    HOMEPAGE_URL https://github.com/HeaoYe/Match
)

option(MATCH_BUILD_EXAMPLES "Build Match Examples" ON)
option(MATCH_SUPPORT_KTX "Support KTX" ON)

add_subdirectory(thirdparty)

add_subdirectory(Match)

if (MATCH_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
