project(GLTF)

file(GLOB_RECURSE source src/*.cpp)

# add_compile_options(-O3)
add_executable(GLTF ${source})

target_compile_definitions(GLTF PRIVATE MATCH_INNER_VISIBLE)
target_link_libraries(GLTF PRIVATE Match)
target_link_libraries(GLTF PRIVATE ktx)
