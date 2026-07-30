#include "server/match/match_lifecycle_handler.h"

#include "app/i_runtime_store.h"
#include "app/runtime_diagnostics.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/game/protocol/game_creation_request.h"
#include "server/game/i_game_allocator.h"
#include "server/gateway/i_game_gateway.h"
#include "server/network/player_id.h"
#include "server/player_session.h"
#include "server/user/user_id.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <utility>

namespace kfc {

MatchLifecycleHandler::MatchLifecycleHandler(IGameAllocator& game_allocator,
                                             IGameRepository& game_repository,
                                             IRuntimeStore& runtime_store, std::string server_id)
    : game_allocator_(game_allocator),
      game_repository_(game_repository),
      runtime_store_(runtime_store),
      server_id_(std::move(server_id)) {}

void MatchLifecycleHandler::bind_matchmaking_service(MatchmakingService& matchmaking_service) {
    matchmaking_service_ = &matchmaking_service;
}

void MatchLifecycleHandler::bind_game_gateway(IGameGateway& game_gateway) {
    game_gateway_ = &game_gateway;
}

RoomId MatchLifecycleHandler::create_match(PlayerSession* white, PlayerSession* black) {
    white->set_playing();
    black->set_playing();
    white->set_side(PieceColor::White);
    black->set_side(PieceColor::Black);

    const std::optional<int> db_game_id =
        game_repository_.create_game(white->player().id(), black->player().id());

    const GameCreationRequest request{white->user_id(), black->user_id(), db_game_id};
    const GameCreationResponse response = game_allocator_.allocate_game(request);
    const RoomId room_id = response.room_id;

    white->assign_room(room_id);
    black->assign_room(room_id);

    const std::string routing_server_id =
        response.game_server_id.empty() ? server_id_ : response.game_server_id;
    const std::string routing_endpoint = resolve_routing(response, white->user_id()).endpoint;
    runtime_store_.register_room(room_id, static_cast<UserId>(white->player().id()),
                               static_cast<UserId>(black->player().id()), routing_server_id,
                               routing_endpoint);

    send_game_redirects(white, black, response);

    return room_id;
}

MatchLifecycleHandler::ResolvedRouting MatchLifecycleHandler::resolve_routing(
    const GameCreationResponse& response, const UserId user_id) const {
    std::string server_id =
        response.game_server_id.empty() ? server_id_ : response.game_server_id;
    std::string endpoint;
    if (response.endpoint.has_value() && !response.endpoint->empty()) {
        endpoint = *response.endpoint;
    } else {
        const std::optional<GameServerLocation> location =
            runtime_store_.find_player_location(user_id);
        if (location.has_value()) {
            if (server_id.empty()) {
                server_id = location->server_id;
            }
            endpoint = location->endpoint;
        }
    }

    return ResolvedRouting{response.room_id, std::move(server_id), std::move(endpoint)};
}

void MatchLifecycleHandler::send_game_redirects(PlayerSession* white, PlayerSession* black,
                                              const GameCreationResponse& response) {
    if (game_gateway_ == nullptr) {
        return;
    }

    const ResolvedRouting routing = resolve_routing(response, white->user_id());
    game_gateway_->send_game_redirect(
        static_cast<PlayerId>(white->id()),
        GatewayGameRedirectInfo{routing.room_id, routing.server_id, routing.endpoint,
                                PieceColor::White});
    game_gateway_->send_game_redirect(
        static_cast<PlayerId>(black->id()),
        GatewayGameRedirectInfo{routing.room_id, routing.server_id, routing.endpoint,
                                PieceColor::Black});
}

void MatchLifecycleHandler::notify_match_created(const MatchCreated& match) {
    if (game_gateway_ == nullptr) {
        return;
    }

    const PlayerId white_id = static_cast<PlayerId>(match.white->id());
    const PlayerId black_id = static_cast<PlayerId>(match.black->id());
    game_gateway_->notify_match_found(white_id, black_id);
    game_gateway_->notify_game_start(white_id, black_id);
}

void MatchLifecycleHandler::process_timeouts() {
    if (matchmaking_service_ == nullptr) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    for (PlayerSession* session : matchmaking_service_->check_timeouts(now)) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cout << "Matchmaking timeout for session " << session->id() << " (player "
                      << session->player().username() << ")\n";
        }
#endif
        if (game_gateway_ != nullptr) {
            game_gateway_->notify_search_timeout(static_cast<PlayerId>(session->id()));
        }
        session->cancel_search();
    }
}

}  // namespace kfc
