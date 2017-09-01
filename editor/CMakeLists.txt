project(editor)

set(SOURCES
    src/editor.cpp
    src/main.cpp)

add_executable(editor ${SOURCES})

target_include_directories(editor PRIVATE include)
target_link_libraries(editor PRIVATE common)
