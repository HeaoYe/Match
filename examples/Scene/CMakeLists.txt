project(Scene)

file(GLOB_RECURSE source src/*.cpp)

add_executable(Scene ${source})
target_link_libraries(Scene PRIVATE Match)
target_link_libraries(Scene PRIVATE glm::glm)
