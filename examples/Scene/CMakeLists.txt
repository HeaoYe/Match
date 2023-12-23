project(Scene)

file(GLOB source src/*.cpp)

add_compile_options(-O3)
add_executable(Scene ${source})

target_link_libraries(Scene PRIVATE Match)
target_link_libraries(Scene PRIVATE glm)
target_link_libraries(Scene PRIVATE tinyobjloader)
