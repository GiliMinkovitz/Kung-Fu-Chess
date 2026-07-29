#include "server/session/client_session_manager.h"

#include "server/client_connection.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/network/player_id.h"
#include "server/session_registry.h"
#include "server/websocket_server.h"

namespace kfc {

ClientSessionManager::ClientSessionManager(WebSocketServer& websocket_server,
                                           SessionRegistry& session_registry,
                                           MatchmakingService& matchmaking_service)
    : websocket_server_(websocket_server),
      session_registry_(session_registry),
      matchmaking_service_(matchmaking_service) {}

PlayerSession* ClientSessionManager::find_session(const PlayerId player_id) noexcept {
    for (PlayerSession& session : sessions_) {
        if (session.id() == player_id) {
            return &session;
        }
    }
    return nullptr;
}

const PlayerSession* ClientSessionManager::find_session(const PlayerId player_id) const noexcept {
    for (const PlayerSession& session : sessions_) {
        if (session.id() == player_id) {
            return &session;
        }
    }
    return nullptr;
}

void ClientSessionManager::accept_new_clients() {
    if (websocket_server_.clients().size() >= websocket_server_.max_clients()) {
        return;
    }

    const std::size_t before = websocket_server_.clients().size();
    websocket_server_.try_accept();
    if (websocket_server_.clients().size() <= before) {
        return;
    }

    ClientConnection& connection = websocket_server_.clients().back();
    sessions_.emplace_back(next_session_id_++, &connection);
}

void ClientSessionManager::prune_sessions() {
    websocket_server_.prune_disconnected();

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (!it->connection()->is_open()) {
            if (it->has_user()) {
                session_registry_.unregister_session(it->player().username());
            }
            matchmaking_service_.remove(*it);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace kfc
