#pragma once

#include <string>

namespace kfc {

class AuthenticationService;
class ClientSessionManager;
class Player;
class PlayerSession;
class SessionRegistry;

namespace matchmaking {
class IMatchmakingJoinClient;
}

class LobbyMessageHandler {
public:
    LobbyMessageHandler(AuthenticationService& authentication_service,
                        matchmaking::IMatchmakingJoinClient& matchmaking_client,
                        SessionRegistry& session_registry,
                        ClientSessionManager& session_manager, std::string region);

    void process();

private:
    void bind_authenticated_user(PlayerSession& session, const Player& authenticated_player);

    AuthenticationService& authentication_service_;
    matchmaking::IMatchmakingJoinClient& matchmaking_client_;
    SessionRegistry& session_registry_;
    ClientSessionManager& session_manager_;
    std::string region_;
};

}  // namespace kfc
