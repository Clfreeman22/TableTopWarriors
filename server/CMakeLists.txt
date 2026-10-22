cmake_minimum_required(VERSION 3.16)

project(TableTopWarriors
    VERSION 0.1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(game_server
    src/main.c
)

target_compile_options(game_server PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)