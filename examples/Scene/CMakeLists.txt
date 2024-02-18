project(Scene)

file(GLOB_RECURSE source CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/*.hpp)

add_compile_options(-O3)
add_executable(Scene ${source} ${headers})

target_link_libraries(Scene PRIVATE Match)
if (WIN32)
    COPYDLL(Scene ../..)
endif()
