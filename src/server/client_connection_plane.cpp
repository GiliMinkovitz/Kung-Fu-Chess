#include "server/client_connection_plane.h"

#include "server/session/i_session_disconnect_handler.h"

namespace kfc {

ClientConnectionPlane::ClientConnectionPlane(const app::ServerConfig& websocket_config,
                                             ISessionDisconnectHandler* disconnect_handler)
    : websocket_server(websocket_config),
      session_manager(websocket_server, session_registry, disconnect_handler),
      session_message_sink(session_manager) {}

}  // namespace kfc
