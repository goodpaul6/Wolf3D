project(game)

set(SOURCES
    src/game.cpp
    src/main.cpp)

add_executable(game ${SOURCES})

target_include_directories(game PRIVATE include)
target_link_libraries(game PRIVATE common)
