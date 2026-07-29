#include "server/lobby/lobby_message_handler.h"

#include "app/observability/correlation_id.h"
#include "app/observability/metric_counters.h"
#include "app/observability/structured_logger.h"
#include "matchmaking/i_matchmaking_join_client.h"
#include "matchmaking/protocol/match_request.h"
#include "server/authentication_service.h"
#include "server/game_message_parser.h"
#include "server/network/player_id.h"
#include "server/player.h"
#include "server/player_session.h"
#include "server/session/client_session_manager.h"
#include "server/session_registry.h"
#include "server/user/user_id.h"

namespace kfc {

LobbyMessageHandler::LobbyMessageHandler(AuthenticationService& authentication_service,
                                         matchmaking::IMatchmakingJoinClient& matchmaking_client,
                                         SessionRegistry& session_registry,
                                         ClientSessionManager& session_manager,
                                         std::string region)
    : authentication_service_(authentication_service),
      matchmaking_client_(matchmaking_client),
      session_registry_(session_registry),
      session_manager_(session_manager),
      region_(std::move(region)) {}

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
                    kfc::app::observability::logger().log(
                        kfc::app::observability::LogLevel::Info, "player_login",
                        {{"username", auth.player->username()},
                         {"player_id", std::to_string(auth.player->id())}});
                }
                continue;
            }

            if (parse_play_message(*raw_message)) {
                session.request_play();
                if (session.state() == PlayerSessionState::Searching) {
                    const matchmaking::MatchRequest request{
                        static_cast<PlayerId>(session.id()), session.user_id(), session.rating(),
                        region_};
                    kfc::app::observability::set_correlation_id(
                        kfc::app::observability::generate_correlation_id());
                    kfc::app::observability::metrics().matchmaking_requests_total.fetch_add(
                        1, std::memory_order_relaxed);
                    if (const std::optional<matchmaking::MatchResponse> response =
                            matchmaking_client_.join(request)) {
                        if (response->status == matchmaking::MatchJoinStatus::Error) {
                            kfc::app::observability::metrics().matchmaking_failures_total.fetch_add(
                                1, std::memory_order_relaxed);
                            session.cancel_search();
                            session.connection()->try_send("search_failed");
                        } else {
                            session.connection()->try_send("searching");
                        }
                    } else {
                        kfc::app::observability::metrics().matchmaking_failures_total.fetch_add(
                            1, std::memory_order_relaxed);
                        session.cancel_search();
                        session.connection()->try_send("search_failed");
                    }
                }
            }
        }
    }
}

}  // namespace kfc
