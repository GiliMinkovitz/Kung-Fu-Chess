cmake_minimum_required(VERSION 3.16)
project(KungFuChess VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the archive file timestamp for FetchContent downloads (CMake 3.24+).
# Avoids unnecessary re-downloads when the URL has not changed.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

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
