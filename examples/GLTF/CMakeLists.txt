project(GLTF)

file(GLOB_RECURSE source src/main.cpp src/camera.cpp)

add_compile_options(-O3)
add_executable(GLTF ${source})

target_compile_definitions(GLTF PRIVATE MATCH_INNER_VISIBLE)
target_link_libraries(GLTF PRIVATE Match)
if (MATCH_SUPPORT_KTX)
    target_link_libraries(GLTF PRIVATE ktx)
endif()
if (WIN32)
    COPYDLL(GLTF ../..)
endif()
