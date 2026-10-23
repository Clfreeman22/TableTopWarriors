cmake_minimum_required(VERSION 3.16)

project(TableTopWarriors
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(game_core STATIC
    src/room.c
)

target_include_directories(game_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(game_core PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_executable(game_server
    src/main.c
)

target_link_libraries(game_server
    PRIVATE game_core
)

enable_testing()

add_executable(test_room
    tests/test_room.c
)

target_link_libraries(test_room PRIVATE game_core)

add_test(
    NAME room_manager_tests
    COMMAND test_room
)