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
    src/logic/piece_rules/bishop_rules.cpp
    src/logic/piece_rules/king_rules.cpp
    src/logic/piece_rules/knight_rules.cpp
    src/logic/piece_rules/pawn_rules.cpp
    src/logic/piece_rules/queen_rules.cpp
    src/logic/piece_rules/rook_rules.cpp
    src/logic/piece_rules/rules_registry.cpp
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
# doctest
# ---------------------------------------------------------------------------

# Prefer a vendored copy (works in WSL when HTTPS/git SSL is blocked, e.g. NetFree).
set(DOCTEST_LOCAL_DIR "${CMAKE_SOURCE_DIR}/third_party/doctest")
if(EXISTS "${DOCTEST_LOCAL_DIR}/doctest/doctest.h")
    message(STATUS "Using local doctest from ${DOCTEST_LOCAL_DIR}")
    set(doctest_SOURCE_DIR "${DOCTEST_LOCAL_DIR}")

    add_library(doctest INTERFACE)
    add_library(doctest::doctest ALIAS doctest)
    target_include_directories(doctest INTERFACE "${DOCTEST_LOCAL_DIR}")

    add_library(doctest_with_main STATIC EXCLUDE_FROM_ALL
        "${DOCTEST_LOCAL_DIR}/doctest/parts/doctest.cpp")
    add_library(doctest::doctest_with_main ALIAS doctest_with_main)
    target_compile_definitions(doctest_with_main PRIVATE DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN)
    set_target_properties(doctest_with_main PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED ON)
    target_link_libraries(doctest_with_main PUBLIC doctest)
else()
    include(FetchContent)

    FetchContent_Declare(
        doctest
        URL https://github.com/doctest/doctest/archive/refs/tags/v2.4.11.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(doctest)
endif()

# doctest_with_main supplies main() that runs all registered TEST_CASE() blocks.
target_link_libraries(KungFuChessTests PRIVATE doctest::doctest_with_main)

enable_testing()

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)

# Register each doctest case with CTest at build time.
doctest_discover_tests(KungFuChessTests)

option(ENABLE_COVERAGE "Enable code coverage instrumentation and LCOV HTML reports" OFF)

if(ENABLE_COVERAGE)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(FATAL_ERROR "ENABLE_COVERAGE requires GCC or Clang (e.g. Linux, WSL, or MinGW)")
    endif()

    foreach(_kfc_target IN ITEMS ${PROJECT_NAME} KungFuChessTests)
        target_compile_options(${_kfc_target} PRIVATE --coverage)
        target_link_options(${_kfc_target} PRIVATE --coverage)
    endforeach()

    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    set(KFC_COVERAGE_REPORT_DIR "${CMAKE_SOURCE_DIR}/coverage_report")

    add_custom_target(coverage
        COMMAND bash -c "rm -rf '${KFC_COVERAGE_REPORT_DIR}' '${CMAKE_BINARY_DIR}/coverage.info'"
        COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --zerocounters
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${LCOV}
            --directory ${CMAKE_BINARY_DIR}
            --capture
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors source
        COMMAND ${LCOV}
            --remove ${CMAKE_BINARY_DIR}/coverage.info
            '/usr/*'
            '*/tests/*'
            '*/_deps/*'
            '*/third_party/*'
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors unused
        COMMAND ${LCOV}
            --extract ${CMAKE_BINARY_DIR}/coverage.info
            '*/src/*'
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors unused,unused
        COMMAND ${GENHTML}
            ${CMAKE_BINARY_DIR}/coverage.info
            --output-directory ${KFC_COVERAGE_REPORT_DIR}
            --ignore-errors source
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS KungFuChessTests
        COMMENT "Generating LCOV HTML coverage report in coverage_report/"
    )
endif()
