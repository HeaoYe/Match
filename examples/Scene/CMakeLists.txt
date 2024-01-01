project(Scene)

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)

add_compile_options(-O3)
add_executable(Scene ${source})

target_link_libraries(Scene PRIVATE Match)
