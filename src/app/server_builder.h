#pragma once

#include "app/app_config.h"
#include "app/server_infrastructure.h"
#include "app/runtime_endpoint.h"
#include "model/board_model.h"
#include "matchmaking/http/matchmaking_http_client.h"
#include "server/client_connection_plane.h"
#include "server/game/local_game_host.h"
#include "server/game/game_allocation_handler.h"
#include "server/game_server.h"
#include "server/gateway_server.h"
#include "server/gateway/gateway_matchmaking_disconnect_handler.h"
#include "server/gateway/gateway_notification_handler.h"
#include "server/gateway/gateway_notification_listener.h"
#include "server/gateway/local_game_gateway.h"
#include "server/game_server_runtime.h"
#include "server/room/room_manager.h"

#include <utility>

namespace kfc::app {

namespace {

ServerConfig make_gateway_websocket_config(const AppConfig& config) {
    ServerConfig websocket_config = config.server;
    websocket_config.port = config.server.port;
    websocket_config.max_clients = config.server.max_clients;
    return websocket_config;
}

ServerConfig make_game_websocket_config(const AppConfig& config) {
    ServerConfig websocket_config = config.server;
    websocket_config.port = config.server.game_port;
    websocket_config.max_clients = config.server.game_max_clients;
    return websocket_config;
}

}  // namespace

struct BuiltGateway {
    ServerInfrastructure infrastructure;
    matchmaking::MatchmakingHttpClient matchmaking_client;
    GatewayMatchmakingDisconnectHandler disconnect_handler;
    ClientConnectionPlane client_plane;
    LocalGameGateway local_game_gateway;
    GatewayNotificationHandler notification_handler;
    GatewayNotificationListener notification_listener;
    GatewayServer gateway;

    BuiltGateway(const AppConfig& config)
        : infrastructure(config),
          matchmaking_client(config.server.matchmaker_endpoint,
                             config.allocation.allocation_timeout),
          disconnect_handler(matchmaking_client, config.server.region),
          client_plane(make_gateway_websocket_config(config), &disconnect_handler),
          local_game_gateway(client_plane.session_message_sink),
          notification_handler(client_plane.session_manager, local_game_gateway),
          notification_listener(config.server.bind_address, config.server.gateway_internal_port,
                                notification_handler),
          gateway(config, client_plane, local_game_gateway, matchmaking_client,
                  infrastructure.authentication_service(), infrastructure.runtime_store(),
                  [this]() { notification_listener.poll(); },
                  [this]() { notification_listener.stop(); }) {}
};

struct BuiltGameServerRuntime {
    ServerInfrastructure infrastructure;
    BoardModel default_board;
    RoomManager room_manager;
    LocalGameHost local_game_host;
    GameAllocationHandler allocation_handler;
    ClientConnectionPlane client_plane;
    GameServerRuntime runtime;

    BuiltGameServerRuntime(const AppConfig& config, BoardModel board)
        : infrastructure(config),
          default_board(std::move(board)),
          room_manager(default_board),
          local_game_host(room_manager, config.server.server_id,
                          resolve_game_endpoint(config.server)),
          allocation_handler(local_game_host, infrastructure.runtime_store(),
                             config.server.server_id, resolve_game_endpoint(config.server)),
          client_plane(make_game_websocket_config(config), nullptr),
          runtime(config, client_plane, room_manager, local_game_host, allocation_handler,
                  infrastructure.user_repository(), infrastructure.game_repository(),
                  infrastructure.runtime_store()) {}
};

struct BuiltGameServer {
    ServerInfrastructure infrastructure;
    GameServer server;

    BuiltGameServer(const AppConfig& config, BoardModel default_board)
        : infrastructure(config),
          server(config, std::move(default_board), infrastructure.dependencies()) {}
};

[[nodiscard]] inline BuiltGameServer build_game_server(const AppConfig& config, BoardModel default_board) {
    return BuiltGameServer{config, std::move(default_board)};
}

[[nodiscard]] inline BuiltGateway build_gateway(const AppConfig& config) {
    return BuiltGateway{config};
}

[[nodiscard]] inline BuiltGameServerRuntime build_game_server_runtime(const AppConfig& config,
                                                                      BoardModel default_board) {
    return BuiltGameServerRuntime{config, std::move(default_board)};
}

}  // namespace kfc::app
