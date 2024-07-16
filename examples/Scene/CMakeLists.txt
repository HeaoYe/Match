project(Scene)

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/*.hpp)

add_compile_options(-O3)
add_executable(Scene ${source} ${headers})

target_link_libraries(Scene PRIVATE Match)

# 转换 openvdb格式
find_package(OpenVDB QUIET)
if (OpenVDB_FOUND)
    target_link_libraries(Scene PRIVATE openvdb Imath tbb)
    target_compile_definitions(Scene PRIVATE WITH_OPENVDB)
endif ()

if (WIN32)
    COPYDLL(Scene ../..)
endif()
