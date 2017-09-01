set(SOURCES
    src/gl3w.c
    src/build.cpp
    src/input.cpp
    src/draw.cpp
    src/graphics.cpp
    src/resources.cpp
    src/context.cpp)

add_library(common STATIC ${SOURCES})

find_package(SDL2 REQUIRED)
find_package(GLM REQUIRED)
find_package(OpenGL REQUIRED)

target_include_directories(common PUBLIC
    ${SDL2_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    include)

target_link_libraries(common PUBLIC
    ${SDL2_LIBRARY}
    ${OPENGL_LIBRARIES}
    ${CMAKE_DL_LIBS})

