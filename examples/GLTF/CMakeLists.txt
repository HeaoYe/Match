project(GLTF)

file(GLOB_RECURSE source src/main.cpp src/camera.cpp)

add_compile_options(-O3)
add_executable(GLTF ${source})

target_compile_definitions(GLTF PRIVATE MATCH_INNER_VISIBLE)
target_link_libraries(GLTF PRIVATE Match)
target_link_libraries(GLTF PRIVATE ktx)
if (WIN32)
    COPYDLL(GLTF ../../..)
endif()
