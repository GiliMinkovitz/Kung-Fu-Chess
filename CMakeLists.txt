cmake_minimum_required(VERSION 3.16)
project(KungFuChess VERSION 0.1.0 LANGUAGES CXX)

message(STATUS "Source directory is: ${CMAKE_SOURCE_DIR}")
file(GLOB_RECURSE ALL_FILES "${CMAKE_SOURCE_DIR}/src/*")
message(STATUS "Found files: ${ALL_FILES}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the archive file timestamp for FetchContent downloads (CMake 3.24+).
# Avoids unnecessary re-downloads when the URL has not changed.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(KFC_SOURCES
    src/core/piece.cpp
    src/core/piece_factory.cpp
    src/core/piece_token.cpp
    src/core/position.cpp
    src/core/board_model.cpp
    src/logic/board_validator.cpp
    src/adapters/board_writer.cpp
    src/logic/path_utils.cpp
    src/adapters/vpl_io.cpp
    src/logic/game_rules.cpp
    src/logic/move_scheduler.cpp
    src/logic/collision_resolver.cpp
    src/logic/real_time_arbiter.cpp
    src/logic/game_state.cpp
    src/adapters/command_processor.cpp
    src/logic/move_validator.cpp
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${KFC_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/adapters
)

set(KFC_TEST_SOURCES
    tests/unit/core/board_model_tests.cpp
    tests/unit/core/piece_tests.cpp
    tests/unit/core/position_tests.cpp
    tests/unit/logic/board_validator_tests.cpp
    tests/unit/logic/path_utils_tests.cpp
    tests/unit/logic/move_validator_tests.cpp
    tests/unit/logic/collision_resolver_tests.cpp
    tests/unit/logic/game_state_tests.cpp
    tests/unit/logic/real_time_arbiter_tests.cpp
    tests/unit/logic/game_rules_tests.cpp
    tests/unit/adapters/board_writer_tests.cpp
    tests/unit/adapters/command_processor_tests.cpp
    tests/unit/adapters/vpl_io_tests.cpp
    tests/integration/integration_test.cpp
)

add_executable(KungFuChessTests
    ${KFC_TEST_SOURCES}
    ${KFC_SOURCES}
)

target_include_directories(KungFuChessTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /permissive-)
    target_compile_options(KungFuChessTests PRIVATE /W4 /permissive-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(KungFuChessTests PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------------------
# doctest (FetchContent)
# ---------------------------------------------------------------------------

# FetchContent lets CMake download third-party code at configure time,
# so contributors do not need to install doctest separately.
include(FetchContent)

# Pin a specific release tag so every configure gets the same doctest version.
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
)

# Download (if needed), add doctest as a subproject, and expose imported
# targets such as doctest::doctest and doctest::doctest_with_main.
FetchContent_MakeAvailable(doctest)

# doctest_with_main supplies main() that runs all registered TEST_CASE() blocks.
target_link_libraries(KungFuChessTests PRIVATE doctest::doctest_with_main)

enable_testing()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# Register each doctest case with CTest at build time.
doctest_discover_tests(KungFuChessTests)

option(KFC_ENABLE_COVERAGE "Generate HTML coverage report with OpenCppCoverage (MSVC only)" OFF)

if(KFC_ENABLE_COVERAGE AND MSVC)
    find_program(OPEN_CPP_COVERAGE OpenCppCoverage)
    if(OPEN_CPP_COVERAGE)
        add_custom_target(coverage
            COMMAND ${OPEN_CPP_COVERAGE}
                --sources ${CMAKE_SOURCE_DIR}/src
                --export_type html:${CMAKE_BINARY_DIR}/coverage_report
                -- $<TARGET_FILE:KungFuChessTests>
            DEPENDS KungFuChessTests
            COMMENT "Running KungFuChessTests with OpenCppCoverage"
        )
    else()
        message(WARNING "OpenCppCoverage not found; install it to use the coverage target")
    endif()
endif()
