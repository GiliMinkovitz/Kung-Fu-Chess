cmake_minimum_required(VERSION 3.16)
project(KungFuChess VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(KFC_SOURCES
    src/piece.cpp
    src/board_model.cpp
    src/board_validator.cpp
    src/board_writer.cpp
    src/path_utils.cpp
    src/vpl_io.cpp
    src/game_rules.cpp
    src/move_scheduler.cpp
    src/collision_resolver.cpp
    src/game_state.cpp
    src/command_processor.cpp
    src/move_validator.cpp
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${KFC_SOURCES}
)

add_executable(KungFuChessTests
    src/tests.cpp
    ${KFC_SOURCES}
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
    target_compile_options(KungFuChessTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(KungFuChessTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(NAME KungFuChessTests COMMAND KungFuChessTests)
