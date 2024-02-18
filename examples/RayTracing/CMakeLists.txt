project(RayTracing)

file(GLOB_RECURSE source src/*.cpp)

add_compile_options(-O3)
add_executable(RayTracing ${source})

target_compile_definitions(RayTracing PRIVATE MATCH_INNER_VISIBLE)
target_link_libraries(RayTracing PRIVATE Match)
if (WIN32)
    COPYDLL(RayTracing)
endif()
