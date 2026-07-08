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
# GoogleTest (FetchContent)
# ---------------------------------------------------------------------------

# FetchContent lets CMake download and build third-party code at configure time,
# so contributors do not need to install GoogleTest separately.
include(FetchContent)

# Pin a specific release tag so every configure gets the same GoogleTest version.
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)

# On MSVC, GoogleTest defaults to a static CRT (/MT). This forces the shared
# CRT (/MD) so it matches typical Visual Studio project settings and avoids
# linker/runtime mismatches when linking KungFuChessTests.
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Download (if needed), add googletest as a subproject, and expose imported
# targets such as GTest::gtest and GTest::gtest_main.
FetchContent_MakeAvailable(googletest)

# GoogleTest ships a CMake module that provides gtest_discover_tests().
include(GoogleTest)

# Link GoogleTest only to the test executable. KungFuChess stays a plain app
# with no test-framework dependency. gtest_main supplies main() that runs all
# registered TEST()/TEST_F() cases (tests.cpp will be migrated to use those).
target_link_libraries(KungFuChessTests PRIVATE GTest::gtest_main)

enable_testing()

# Register each GoogleTest case with CTest at build time (replaces a single
# monolithic add_test that ran the whole executable as one test).
gtest_discover_tests(KungFuChessTests)

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
