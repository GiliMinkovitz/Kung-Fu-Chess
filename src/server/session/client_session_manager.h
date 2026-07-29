#pragma once

#include "server/player_session.h"

#include "server/network/player_id.h"

#include <cstddef>
#include <list>

namespace kfc {

class MatchmakingService;
class SessionRegistry;
class WebSocketServer;

class ClientSessionManager {
public:
    ClientSessionManager(WebSocketServer& websocket_server, SessionRegistry& session_registry,
                         MatchmakingService& matchmaking_service);

    void accept_new_clients();
    void prune_sessions();

    [[nodiscard]] PlayerSession* find_session(PlayerId player_id) noexcept;
    [[nodiscard]] const PlayerSession* find_session(PlayerId player_id) const noexcept;

    [[nodiscard]] std::list<PlayerSession>& sessions() noexcept { return sessions_; }
    [[nodiscard]] const std::list<PlayerSession>& sessions() const noexcept { return sessions_; }

private:
    WebSocketServer& websocket_server_;
    SessionRegistry& session_registry_;
    MatchmakingService& matchmaking_service_;
    std::list<PlayerSession> sessions_;
    std::size_t next_session_id_ = 0;
};

}  // namespace kfc
