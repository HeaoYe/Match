project(Sandbox)

file(GLOB_RECURSE source src/*.cpp)

add_executable(Sandbox ${source})
target_link_libraries(Sandbox PRIVATE Match)
target_link_libraries(Sandbox PRIVATE glm::glm)
target_include_directories(Sandbox PRIVATE "${GLFW_SOURCE_DIR}/deps")
target_link_libraries(Sandbox PRIVATE glfw)
