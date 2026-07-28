#include "server/lobby/lobby_message_handler.h"

#include "server/authentication_service.h"
#include "server/game_message_parser.h"
#include "server/match/match_lifecycle_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/player.h"
#include "server/player_session.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/user/user_id.h"

#include <chrono>

namespace kfc {

LobbyMessageHandler::LobbyMessageHandler(AuthenticationService& authentication_service,
                                         MatchmakingService& matchmaking_service,
                                         SessionRegistry& session_registry,
                                         ClientSessionManager& session_manager,
                                         MatchLifecycleHandler& match_lifecycle_handler)
    : authentication_service_(authentication_service),
      matchmaking_service_(matchmaking_service),
      session_registry_(session_registry),
      session_manager_(session_manager),
      match_lifecycle_handler_(match_lifecycle_handler) {}

void LobbyMessageHandler::bind_authenticated_user(PlayerSession& session,
                                                    const Player& authenticated_player) {
    session.assign_user(static_cast<UserId>(authenticated_player.id()), authenticated_player.username(),
                        authenticated_player.rating());
}

void LobbyMessageHandler::process() {
    for (PlayerSession& session : session_manager_.sessions()) {
        if (session.state() == PlayerSessionState::Playing ||
            !session.connection()->is_open()) {
            continue;
        }

        if (const auto raw_message = session.connection()->try_read()) {
            if (session.state() == PlayerSessionState::Searching) {
                parse_play_message(*raw_message);
                continue;
            }

            if (!session.has_user()) {
                if (const auto request = parse_login_message(*raw_message)) {
                    if (session_registry_.is_online(request->username)) {
                        session.connection()->try_send("login_failed already_connected");
                        continue;
                    }

                    const AuthenticationResult auth =
                        authentication_service_.authenticate(request->username, request->password);
                    if (!auth.success) {
                        session.connection()->try_send("login_failed " + auth.failure_reason);
                        continue;
                    }

                    bind_authenticated_user(session, *auth.player);
                    session_registry_.register_session(auth.player->username());
                    session.connection()->try_send("login_ok " + std::to_string(auth.player->rating()));
                }
                continue;
            }

            if (parse_play_message(*raw_message)) {
                session.request_play();
                if (session.state() == PlayerSessionState::Searching) {
                    const auto now = std::chrono::steady_clock::now();
                    if (const auto match = matchmaking_service_.enqueue(session, now)) {
                        match_lifecycle_handler_.notify_match_created(*match);
                    } else {
                        session.connection()->try_send("searching");
                    }
                }
            }
        }
    }
}

}  // namespace kfc
