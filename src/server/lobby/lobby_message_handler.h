#pragma once

namespace kfc {

class AuthenticationService;
class ClientSessionManager;
class MatchCreated;
class MatchLifecycleHandler;
class MatchmakingService;
class Player;
class PlayerSession;
class SessionRegistry;

class LobbyMessageHandler {
public:
    LobbyMessageHandler(AuthenticationService& authentication_service,
                        MatchmakingService& matchmaking_service,
                        SessionRegistry& session_registry,
                        ClientSessionManager& session_manager,
                        MatchLifecycleHandler& match_lifecycle_handler);

    void process();

private:
    void bind_authenticated_user(PlayerSession& session, const Player& authenticated_player);

    AuthenticationService& authentication_service_;
    MatchmakingService& matchmaking_service_;
    SessionRegistry& session_registry_;
    ClientSessionManager& session_manager_;
    MatchLifecycleHandler& match_lifecycle_handler_;
};

}  // namespace kfc
