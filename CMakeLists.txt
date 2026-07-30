cmake_minimum_required(VERSION 3.16)

# vcpkg manifest mode (Windows): use CMakePresets.json preset "windows-vcpkg-gui" for
# KungFuChessGui (vcpkg-gui manifest: Boost only) or "windows-vcpkg-server" for server
# targets (vcpkg-server manifest: Boost, libpq, hiredis, sqlite3). Pass
# -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake and
# -DVCPKG_MANIFEST_DIR=<vcpkg-gui|vcpkg-server> before project(). Linux/WSL builds
# omit the vcpkg toolchain and use system packages.

project(KungFuChess VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use the archive file timestamp for FetchContent downloads (CMake 3.24+).
# Avoids unnecessary re-downloads when the URL has not changed.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

set(KFC_UI_SOURCES
    src/ui/assets/asset_paths.cpp
    src/ui/layout/board_layout_calculator.cpp
    src/ui/view/board_view_model.cpp
    src/ui/view/board_view_builder.cpp
    src/ui/view/piece_sprite_selector.cpp
    src/ui/rendering/i_ui_input_sink.cpp
    src/ui/rendering/i_ui_renderer.cpp
    src/ui/rendering/headless_renderer.cpp
    src/ui/controller/ui_controller.cpp
)

set(KFC_GUI_SOURCES
    src/ui/rendering/ctd26_renderer.cpp
    src/ui/assets/image_loader.cpp
    third_party/CTD26/cpp/src/img.cpp
)

set(KFC_NETWORK_SOURCES
    src/network/websocket_client.cpp
    src/network/snapshot_reader.cpp
    src/network/matchmaking_message_reader.cpp
    src/network/login_message_reader.cpp
    src/network/game_result_message_reader.cpp
    src/network/network_input_handler.cpp
    src/network/network_login_session.cpp
    src/network/game_redirect_message_reader.cpp
    src/network/websocket_endpoint.cpp
    src/network/network_game_redirect_flow.cpp
)

set(KFC_SERVER_WEBSOCKET_SOURCES
    src/server/client_connection.cpp
    src/server/websocket_server.cpp
)

set(KFC_APP_SOURCES
    src/app/app_config.cpp
    src/app/config_loader.cpp
    src/app/database_config.cpp
    src/app/health_http_server.cpp
    src/app/in_memory_runtime_store.cpp
    src/app/runtime_diagnostics.cpp
    src/app/server_health.cpp
    src/app/server_infrastructure.cpp
    src/app/observability/correlation_id.cpp
    src/app/observability/metric_counters.cpp
    src/app/observability/observability.cpp
    src/app/observability/prometheus_formatter.cpp
    src/app/observability/readiness_checker.cpp
    src/app/observability/structured_logger.cpp
)

set(KFC_SERVER_CORE_SOURCES
    src/server/player.cpp
    src/server/player_session.cpp
    src/server/user/user.cpp
    src/server/database/in_memory_user_repository.cpp
    src/server/database/postgres_user_repository.cpp
    src/server/database/sqlite_user_repository.cpp
    src/server/game_message_parser.cpp
    src/server/rating_service.cpp
    src/server/password_hasher.cpp
    src/server/authentication_service.cpp
    src/server/session_registry.cpp
    src/server/session/client_session_manager.cpp
    src/server/network/session_message_sink.cpp
    src/server/client_connection_plane.cpp
)

set(KFC_GATEWAY_LOBBY_SOURCES
    src/server/matchmaking/matchmaking_queue.cpp
    src/server/matchmaking/matchmaking_service.cpp
    src/server/lobby/lobby_message_handler.cpp
    src/server/gateway_server.cpp
    src/server/gateway/local_game_gateway.cpp
    src/server/match/match_lifecycle_handler.cpp
)

set(KFC_SERVER_SHARED_SOURCES
    ${KFC_SERVER_CORE_SOURCES}
    ${KFC_GATEWAY_LOBBY_SOURCES}
)

set(KFC_MATCHMAKING_CODEC_SOURCES
    src/matchmaking/protocol/matchmaking_codec.cpp
)

set(KFC_SERVER_INFRA_SOURCES
    src/server/player.cpp
    src/server/user/user.cpp
    src/server/database/sqlite_user_repository.cpp
    src/server/database/postgres_user_repository.cpp
    src/server/password_hasher.cpp
    src/server/authentication_service.cpp
)

set(KFC_DISTRIBUTED_GATEWAY_SOURCES
    src/server/gateway/gateway_notification_handler.cpp
    src/server/gateway/gateway_notification_listener.cpp
    src/server/gateway/gateway_matchmaking_disconnect_handler.cpp
    src/matchmaking/http/matchmaking_http_client.cpp
)

set(KFC_MONOLITH_EXTRA_SOURCES
    src/server/game/local_game_allocator.cpp
    src/server/gateway/monolith_matchmaking_disconnect_handler.cpp
    src/server/gateway_server_monolith.cpp
    src/matchmaking/local_matchmaking_join_client.cpp
)

set(KFC_MATCHMAKER_ONLY_SOURCES
    src/server/game/remote_game_allocator.cpp
    src/server/game/runtime_store_game_server_registry.cpp
    src/server/game/game_allocation_http_client.cpp
    src/matchmaking/player_matchmaking_queue.cpp
    src/matchmaking/remote_matchmaking_service.cpp
    src/matchmaking/http/matchmaking_http_server.cpp
    src/matchmaking/http/gateway_notifier_http_client.cpp
    src/matchmaking/matchmaker_runtime.cpp
)

set(KFC_MATCHMAKER_RUNTIME_SOURCES
    ${KFC_MATCHMAKER_ONLY_SOURCES}
    ${KFC_MATCHMAKING_CODEC_SOURCES}
    src/server/game/protocol/game_creation_codec.cpp
)

set(KFC_INTEGRATION_ALLOCATION_SOURCES
    src/server/game/local_game_allocator.cpp
    src/server/game/remote_game_allocator.cpp
    src/server/game/runtime_store_game_server_registry.cpp
    src/server/game/game_allocation_http_client.cpp
)

set(KFC_GAME_SERVER_RUNTIME_SOURCES
    src/server/room/room.cpp
    src/server/room/room_manager.cpp
    src/server/room/active_room_processor.cpp
    src/server/game_result/game_result_handler.cpp
    src/server/network/game_input_dispatcher.cpp
    src/server/game_server_runtime.cpp
    src/server/game/local_game_host.cpp
    src/server/game/game_allocation_handler.cpp
    src/server/game/game_creation_listener.cpp
    src/server/game/game_join_handler.cpp
    src/server/gateway/local_game_completion_gateway.cpp
    src/server/game_result_message_writer.cpp
    src/server/game/protocol/game_creation_codec.cpp
)

set(KFC_SERVER_ARCH_SOURCES
    ${KFC_SERVER_SHARED_SOURCES}
    ${KFC_MONOLITH_EXTRA_SOURCES}
    ${KFC_MATCHMAKING_CODEC_SOURCES}
    ${KFC_GAME_SERVER_RUNTIME_SOURCES}
    src/server/game_server.cpp
)

set(KFC_DATABASE_SOURCES
    src/database/sqlite_database.cpp
    src/database/player_repository.cpp
    src/database/game_repository.cpp
    src/database/postgres_game_repository.cpp
)

function(kfc_find_postgresql out_has_libpq out_libraries out_include_dirs)
    set(_has_libpq FALSE)
    set(_libraries "")
    set(_include_dirs "")

    find_package(PostgreSQL QUIET)
    if(TARGET PostgreSQL::PostgreSQL)
        set(_has_libpq TRUE)
        set(_libraries PostgreSQL::PostgreSQL)
    elseif(PostgreSQL_FOUND AND PostgreSQL_LIBRARIES)
        set(_has_libpq TRUE)
        set(_libraries ${PostgreSQL_LIBRARIES})
        set(_include_dirs ${PostgreSQL_INCLUDE_DIRS})
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBPQ QUIET libpq)
            if(LIBPQ_FOUND)
                set(_has_libpq TRUE)
                set(_libraries ${LIBPQ_LIBRARIES})
                set(_include_dirs ${LIBPQ_INCLUDE_DIRS})
            endif()
        endif()
    endif()

    set(${out_has_libpq} ${_has_libpq} PARENT_SCOPE)
    set(${out_libraries} ${_libraries} PARENT_SCOPE)
    set(${out_include_dirs} ${_include_dirs} PARENT_SCOPE)
endfunction()

function(kfc_find_hiredis out_has_hiredis out_libraries out_include_dirs)
    set(_has_hiredis FALSE)
    set(_libraries "")
    set(_include_dirs "")

    find_package(hiredis QUIET)
    if(hiredis_FOUND)
        set(_has_hiredis TRUE)
        if(TARGET hiredis::hiredis)
            set(_libraries hiredis::hiredis)
        elseif(HIREDIS_LIBRARIES)
            set(_libraries ${HIREDIS_LIBRARIES})
            set(_include_dirs ${HIREDIS_INCLUDE_DIRS})
        endif()
    else()
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(HIREDIS QUIET hiredis)
            if(HIREDIS_FOUND)
                set(_has_hiredis TRUE)
                set(_libraries ${HIREDIS_LIBRARIES})
                set(_include_dirs ${HIREDIS_INCLUDE_DIRS})
            endif()
        endif()
    endif()

    set(${out_has_hiredis} ${_has_hiredis} PARENT_SCOPE)
    set(${out_libraries} ${_libraries} PARENT_SCOPE)
    set(${out_include_dirs} ${_include_dirs} PARENT_SCOPE)
endfunction()

kfc_find_postgresql(KFC_HAS_LIBPQ KFC_LIBPQ_LIBRARIES KFC_LIBPQ_INCLUDE_DIRS)
kfc_find_hiredis(KFC_HAS_HIREDIS KFC_HIREDIS_LIBRARIES KFC_HIREDIS_INCLUDE_DIRS)

if(KFC_HAS_LIBPQ)
    message(STATUS "Found libpq; building PostgreSQL connection support")
    list(APPEND KFC_DATABASE_SOURCES src/database/postgres_connection.cpp)
else()
    message(STATUS "libpq not found; building PostgreSQL connection stub")
    list(APPEND KFC_DATABASE_SOURCES src/database/postgres_connection_stub.cpp)
endif()

if(KFC_HAS_HIREDIS)
    message(STATUS "Found hiredis; building Redis runtime store support")
    list(APPEND KFC_APP_SOURCES src/app/redis_runtime_store.cpp)
else()
    message(STATUS "hiredis not found; building Redis runtime store stub")
    list(APPEND KFC_APP_SOURCES src/app/redis_runtime_store_stub.cpp)
endif()

function(kfc_link_postgresql target)
    if(NOT KFC_HAS_LIBPQ)
        target_compile_definitions(${target} PRIVATE KFC_HAS_LIBPQ=0)
        return()
    endif()

    target_compile_definitions(${target} PRIVATE KFC_HAS_LIBPQ=1)
    if(TARGET PostgreSQL::PostgreSQL)
        target_link_libraries(${target} PRIVATE PostgreSQL::PostgreSQL)
        return()
    endif()

    if(KFC_LIBPQ_INCLUDE_DIRS)
        target_include_directories(${target} PRIVATE ${KFC_LIBPQ_INCLUDE_DIRS})
    endif()
    target_link_libraries(${target} PRIVATE ${KFC_LIBPQ_LIBRARIES})
endfunction()

function(kfc_link_hiredis target)
    if(NOT KFC_HAS_HIREDIS)
        target_compile_definitions(${target} PRIVATE KFC_HAS_HIREDIS=0)
        return()
    endif()

    target_compile_definitions(${target} PRIVATE KFC_HAS_HIREDIS=1)
    if(TARGET hiredis::hiredis)
        target_link_libraries(${target} PRIVATE hiredis::hiredis)
        return()
    endif()

    if(KFC_HIREDIS_INCLUDE_DIRS)
        target_include_directories(${target} PRIVATE ${KFC_HIREDIS_INCLUDE_DIRS})
    endif()
    target_link_libraries(${target} PRIVATE ${KFC_HIREDIS_LIBRARIES})
endfunction()

set(KFC_CTD26_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/third_party/CTD26/cpp/src")

set(KFC_SOURCES
    src/model/piece.cpp
    src/model/piece_factory.cpp
    src/model/piece_token.cpp
    src/model/position.cpp
    src/model/board_model.cpp
    src/rules/board_validator.cpp
    src/io/board_writer.cpp
    src/logic/path_utils.cpp
    src/rules/piece_rules/bishop_rules.cpp
    src/rules/piece_rules/king_rules.cpp
    src/rules/piece_rules/knight_rules.cpp
    src/rules/piece_rules/pawn_rules.cpp
    src/rules/piece_rules/queen_rules.cpp
    src/rules/piece_rules/rook_rules.cpp
    src/rules/rules_registry.cpp
    src/io/vpl_io.cpp
    src/rules/game_rules.cpp
    src/realtime/move_scheduler.cpp
    src/realtime/collision_resolver.cpp
    src/realtime/real_time_arbiter.cpp
    src/logic/game_state.cpp
    src/io/game_input_handler.cpp
    src/rules/move_validator.cpp
    src/engine/game_engine.cpp
    src/server/match.cpp
    ${KFC_UI_SOURCES}
)

add_executable(${PROJECT_NAME}
    src/main.cpp
    ${KFC_SOURCES}
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logic
    ${CMAKE_CURRENT_SOURCE_DIR}/src/realtime
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rules
    ${CMAKE_CURRENT_SOURCE_DIR}/src/io
    ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/src/server
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)

add_executable(KungFuChessServer
    src/main_server.cpp
    src/server/client_connection.cpp
    src/server/snapshot_writer.cpp
    src/server/websocket_server.cpp
    ${KFC_APP_SOURCES}
    ${KFC_SERVER_ARCH_SOURCES}
    ${KFC_DATABASE_SOURCES}
    ${KFC_SOURCES}
)

add_executable(KungFuChessGateway
    src/main_gateway.cpp
    src/server/client_connection.cpp
    src/server/websocket_server.cpp
    ${KFC_APP_SOURCES}
    ${KFC_SERVER_CORE_SOURCES}
    src/server/lobby/lobby_message_handler.cpp
    src/server/gateway_server.cpp
    src/server/gateway/local_game_gateway.cpp
    ${KFC_DISTRIBUTED_GATEWAY_SOURCES}
    ${KFC_MATCHMAKING_CODEC_SOURCES}
    ${KFC_DATABASE_SOURCES}
    ${KFC_SOURCES}
)

add_executable(KungFuChessMatchmaker
    src/main_matchmaker.cpp
    ${KFC_APP_SOURCES}
    ${KFC_SERVER_INFRA_SOURCES}
    ${KFC_MATCHMAKER_RUNTIME_SOURCES}
    ${KFC_DATABASE_SOURCES}
    ${KFC_SOURCES}
)

add_executable(KungFuChessGameServer
    src/main_game_server.cpp
    src/server/client_connection.cpp
    src/server/snapshot_writer.cpp
    src/server/websocket_server.cpp
    ${KFC_APP_SOURCES}
    ${KFC_SERVER_CORE_SOURCES}
    ${KFC_GAME_SERVER_RUNTIME_SOURCES}
    ${KFC_DATABASE_SOURCES}
    ${KFC_SOURCES}
)

# Boost (Asio/Beast). On Windows, vcpkg-gui or vcpkg-server manifests supply these.
# Boost 1.89+ removed the boost_system CMake package (header-only since 1.69).
find_package(Boost 1.74 REQUIRED CONFIG OPTIONAL_COMPONENTS system)

function(kfc_link_boost target)
    if(TARGET Boost::headers)
        target_link_libraries(${target} PRIVATE Boost::headers)
    else()
        target_include_directories(${target} PRIVATE ${Boost_INCLUDE_DIRS})
    endif()

    if(TARGET Boost::system)
        target_link_libraries(${target} PRIVATE Boost::system)
    endif()

    if(WIN32)
        target_link_libraries(${target} PRIVATE ws2_32 wsock32)
    endif()
endfunction()

function(kfc_link_sqlite target)
    find_package(SQLite3 QUIET)
    if(TARGET SQLite3::SQLite3)
        target_link_libraries(${target} PRIVATE SQLite3::SQLite3)
        return()
    endif()

    if(SQLite3_FOUND AND SQLite3_LIBRARIES)
        target_include_directories(${target} PRIVATE ${SQLite3_INCLUDE_DIRS})
        target_link_libraries(${target} PRIVATE ${SQLite3_LIBRARIES})
        return()
    endif()

    set(KFC_SQLITE_AMALG_DIR "${CMAKE_SOURCE_DIR}/third_party/sqlite")
    if(EXISTS "${KFC_SQLITE_AMALG_DIR}/sqlite3.c")
        if(NOT TARGET kfc_sqlite3)
            message(STATUS "Using bundled SQLite amalgamation from ${KFC_SQLITE_AMALG_DIR}")
            add_library(kfc_sqlite3 STATIC "${KFC_SQLITE_AMALG_DIR}/sqlite3.c")
            target_include_directories(kfc_sqlite3 PUBLIC "${KFC_SQLITE_AMALG_DIR}")
            if(MSVC)
                target_compile_options(kfc_sqlite3 PRIVATE /W0)
            else()
                target_compile_options(kfc_sqlite3 PRIVATE -w)
            endif()
        endif()
        target_link_libraries(${target} PRIVATE kfc_sqlite3)
        return()
    endif()

    message(FATAL_ERROR
        "SQLite3 not found. Install libsqlite3-dev (Linux/WSL), add sqlite3 via vcpkg (Windows), "
        "or vendor the amalgamation at third_party/sqlite/sqlite3.c")
endfunction()

function(kfc_configure_server_target target)
    target_include_directories(${target} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/model
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logic
        ${CMAKE_CURRENT_SOURCE_DIR}/src/realtime
        ${CMAKE_CURRENT_SOURCE_DIR}/src/rules
        ${CMAKE_CURRENT_SOURCE_DIR}/src/io
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
        ${CMAKE_CURRENT_SOURCE_DIR}/src/server
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    )

    kfc_link_boost(${target})
    kfc_link_sqlite(${target})
    kfc_link_postgresql(${target})
    kfc_link_hiredis(${target})

    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

kfc_configure_server_target(KungFuChessServer)
kfc_configure_server_target(KungFuChessGateway)
kfc_configure_server_target(KungFuChessGameServer)
kfc_configure_server_target(KungFuChessMatchmaker)

# ---------------------------------------------------------------------------
# Windows GUI (OpenCV + CTD26)
# ---------------------------------------------------------------------------

option(KFC_BUILD_GUI "Build KungFuChessGui with OpenCV/CTD26 (Windows only)" OFF)

set(KFC_OPENCV_DIR "${CMAKE_SOURCE_DIR}/third_party/CTD26/cpp/OpenCV_451")
if(WIN32 AND EXISTS "${KFC_OPENCV_DIR}/include/opencv2/opencv.hpp")
    set(KFC_BUILD_GUI ON CACHE BOOL "Build KungFuChessGui with OpenCV/CTD26 (Windows only)" FORCE)
endif()

if(KFC_BUILD_GUI)
    if(NOT WIN32)
        message(FATAL_ERROR "KFC_BUILD_GUI requires Windows and the vendored OpenCV_451 bundle.")
    endif()

    set(KFC_OPENCV_INCLUDE_DIR "${KFC_OPENCV_DIR}/include")
    set(KFC_OPENCV_LIB_DIR "${KFC_OPENCV_DIR}/bin")

    add_executable(KungFuChessGui
        src/main_gui.cpp
        ${KFC_SOURCES}
        ${KFC_GUI_SOURCES}
        ${KFC_NETWORK_SOURCES}
    )

    target_include_directories(KungFuChessGui PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${KFC_CTD26_INCLUDE_DIR}
        ${KFC_OPENCV_INCLUDE_DIR}
    )

    target_link_directories(KungFuChessGui PRIVATE ${KFC_OPENCV_LIB_DIR})
    target_link_libraries(KungFuChessGui PRIVATE
        $<$<CONFIG:Debug>:${KFC_OPENCV_LIB_DIR}/opencv_world451d.lib>
        $<$<NOT:$<CONFIG:Debug>>:${KFC_OPENCV_LIB_DIR}/opencv_world451.lib>
    )

    add_custom_command(TARGET KungFuChessGui POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<$<CONFIG:Debug>:${KFC_OPENCV_LIB_DIR}/opencv_world451d.dll>
        $<$<NOT:$<CONFIG:Debug>>:${KFC_OPENCV_LIB_DIR}/opencv_world451.dll>
        $<TARGET_FILE_DIR:KungFuChessGui>
    )

    kfc_link_boost(KungFuChessGui)

    if(MSVC)
        target_compile_options(KungFuChessGui PRIVATE /W4 /permissive-)
    else()
        target_compile_options(KungFuChessGui PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    message(STATUS "KungFuChessGui enabled (OpenCV at ${KFC_OPENCV_DIR})")
else()
    message(STATUS "KungFuChessGui disabled (OpenCV not found at ${KFC_OPENCV_DIR})")
endif()

set(KFC_TEST_SOURCES
    tests/unit/model/board_model_tests.cpp
    tests/unit/model/piece_tests.cpp
    tests/unit/model/position_tests.cpp
    tests/unit/rules/board_validator_tests.cpp
    tests/unit/logic/path_utils_tests.cpp
    tests/unit/rules/move_validator_tests.cpp
    tests/unit/realtime/collision_resolver_tests.cpp
    tests/unit/logic/game_state_tests.cpp
    tests/unit/logic/game_action_tests.cpp
    tests/unit/realtime/move_scheduler_tests.cpp
    tests/unit/realtime/real_time_arbiter_tests.cpp
    tests/unit/rules/game_rules_tests.cpp
    tests/unit/io/board_writer_tests.cpp
    tests/unit/io/game_input_handler_tests.cpp
    tests/unit/io/vpl_io_tests.cpp
    tests/unit/engine/game_engine_tests.cpp
    tests/unit/ui/board_layout_calculator_tests.cpp
    tests/unit/ui/board_view_builder_tests.cpp
    tests/unit/ui/board_view_model_tests.cpp
    tests/unit/app/config_loader_tests.cpp
    tests/unit/app/in_memory_runtime_store_tests.cpp
    tests/unit/app/database_config_tests.cpp
    tests/unit/app/health_http_server_tests.cpp
    tests/unit/app/server_health_tests.cpp
    tests/unit/ui/asset_paths_tests.cpp
    tests/unit/ui/piece_sprite_selector_tests.cpp
    tests/unit/ui/ui_controller_tests.cpp
    tests/unit/network/websocket_client_tests.cpp
    tests/unit/network/snapshot_reader_tests.cpp
    tests/unit/network/network_input_handler_tests.cpp
    tests/unit/network/network_login_session_tests.cpp
    tests/unit/database/sqlite_database_tests.cpp
    tests/unit/database/postgres_connection_tests.cpp
    tests/unit/database/postgres_game_repository_tests.cpp
    tests/unit/database/postgres_repository_integration_tests.cpp
    tests/unit/database/player_repository_tests.cpp
    tests/unit/database/game_repository_tests.cpp
    tests/unit/server/matchmaking_tests.cpp
    tests/unit/server/matchmaking_service_tests.cpp
    tests/unit/server/local_game_gateway_tests.cpp
    tests/unit/server/match_lifecycle_handler_tests.cpp
    tests/unit/server/game_creation_codec_tests.cpp
    tests/unit/server/runtime_store_game_server_registry_tests.cpp
    tests/unit/server/room_tests.cpp
    tests/unit/server/room_manager_tests.cpp
    tests/unit/server/match_tests.cpp
    tests/unit/server/player_session_tests.cpp
    tests/unit/server/user_tests.cpp
    tests/unit/server/user_repository_tests.cpp
    tests/unit/server/postgres_user_repository_tests.cpp
    tests/unit/server/client_connection_tests.cpp
    tests/unit/server/client_connection_hook_tests.cpp
    tests/unit/server/game_message_parser_tests.cpp
    tests/unit/server/rating_service_tests.cpp
    tests/unit/server/authentication_service_tests.cpp
    tests/unit/server/password_hasher_tests.cpp
    tests/unit/server/session_registry_tests.cpp
    tests/unit/server/game_server_tests.cpp
    tests/unit/server/game_server_multi_room_tests.cpp
    tests/unit/server/websocket_server_tests.cpp
    tests/unit/network/matchmaking_message_reader_tests.cpp
    tests/unit/network/game_redirect_message_reader_tests.cpp
    tests/unit/network/network_game_redirect_flow_tests.cpp
    tests/unit/network/login_message_reader_tests.cpp
    tests/unit/network/game_result_message_reader_tests.cpp
    tests/integration/integration_test.cpp
    tests/integration/allocation_integration_test.cpp
)

add_executable(KungFuChessTests
    ${KFC_TEST_SOURCES}
    ${KFC_SOURCES}
    ${KFC_NETWORK_SOURCES}
    ${KFC_SERVER_WEBSOCKET_SOURCES}
    ${KFC_APP_SOURCES}
    ${KFC_SERVER_ARCH_SOURCES}
    ${KFC_DISTRIBUTED_GATEWAY_SOURCES}
    ${KFC_MATCHMAKER_ONLY_SOURCES}
    ${KFC_DATABASE_SOURCES}
    src/server/snapshot_writer.cpp
)

target_include_directories(KungFuChessTests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

target_compile_definitions(KungFuChessTests PRIVATE KFC_TEST_BUILD)

kfc_link_boost(KungFuChessTests)
kfc_link_sqlite(KungFuChessTests)
kfc_link_postgresql(KungFuChessTests)
kfc_link_hiredis(KungFuChessTests)

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
        target_compile_options(${_kfc_target} PRIVATE --coverage -fprofile-update=atomic -O0 -fno-elide-constructors)
        target_link_options(${_kfc_target} PRIVATE --coverage)
    endforeach()

    find_program(LCOV lcov REQUIRED)
    find_program(GENHTML genhtml REQUIRED)

    set(KFC_COVERAGE_REPORT_DIR "${CMAKE_SOURCE_DIR}/coverage_report")

    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target KungFuChessTests
        COMMAND ${CMAKE_COMMAND} -E rm -rf "${KFC_COVERAGE_REPORT_DIR}" "${CMAKE_BINARY_DIR}/coverage.info"
        COMMAND bash -c "find '${CMAKE_BINARY_DIR}' -name '*.gcda' -delete"
        COMMAND ${LCOV} --directory ${CMAKE_BINARY_DIR} --zerocounters
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:KungFuChessTests>" /tmp/KungFuChessTests_cov
        COMMAND chmod +x /tmp/KungFuChessTests_cov
        COMMAND /tmp/KungFuChessTests_cov
        COMMAND ${CMAKE_COMMAND} -E rm -f /tmp/KungFuChessTests_cov
        COMMAND ${LCOV}
            --directory ${CMAKE_BINARY_DIR}
            --capture
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors source,inconsistent,negative
            --rc geninfo_unexecuted_blocks=1
        COMMAND ${LCOV}
            --remove ${CMAKE_BINARY_DIR}/coverage.info
            '/usr/*'
            '*/tests/*'
            '*/_deps/*'
            '*/third_party/*'
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors unused,inconsistent
        COMMAND ${LCOV}
            --extract ${CMAKE_BINARY_DIR}/coverage.info
            '*/src/*'
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors unused,inconsistent
        COMMAND ${LCOV}
            --remove ${CMAKE_BINARY_DIR}/coverage.info
            '*/src/ui/rendering/i_ui_input_sink.cpp'
            '*/src/ui/rendering/i_ui_renderer.cpp'
            --output-file ${CMAKE_BINARY_DIR}/coverage.info
            --ignore-errors unused,inconsistent
        COMMAND ${GENHTML}
            ${CMAKE_BINARY_DIR}/coverage.info
            --output-directory ${KFC_COVERAGE_REPORT_DIR}
            --ignore-errors source
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        USES_TERMINAL
        COMMENT "Generating LCOV HTML coverage report in coverage_report/"
    )
endif()
