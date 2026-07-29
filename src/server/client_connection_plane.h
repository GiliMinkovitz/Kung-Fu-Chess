#pragma once

#include "app/server_config.h"
#include "server/network/session_message_sink.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/websocket_server.h"

namespace kfc {

class ISessionDisconnectHandler;

struct ClientConnectionPlane {
    WebSocketServer websocket_server;
    SessionRegistry session_registry;
    ClientSessionManager session_manager;
    SessionMessageSink session_message_sink;

    ClientConnectionPlane(const app::ServerConfig& websocket_config,
                          ISessionDisconnectHandler* disconnect_handler = nullptr);
};

}  // namespace kfc
