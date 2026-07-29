#include "server/match/match_lifecycle_handler.h"

#include "app/i_runtime_store.h"
#include "app/runtime_diagnostics.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/network/player_id.h"
#include "server/game/i_game_host.h"
#include "server/gateway/i_game_gateway.h"
#include "server/player_session.h"
#include "server/room/game_player.h"
#include "server/user/user_id.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <utility>

namespace kfc {

namespace {

GamePlayer to_game_player(const PlayerSession& session, const PieceColor side) {
    return GamePlayer{session.user_id(), side, static_cast<PlayerId>(session.id())};
}

}  // namespace

MatchLifecycleHandler::MatchLifecycleHandler(IGameHost& game_host,
                                             IGameRepository& game_repository,
                                             IRuntimeStore& runtime_store, std::string server_id)
    : game_host_(game_host),
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

    const GamePlayer white_player = to_game_player(*white, PieceColor::White);
    const GamePlayer black_player = to_game_player(*black, PieceColor::Black);

    const std::optional<int> db_game_id =
        game_repository_.create_game(white->player().id(), black->player().id());

    const RoomId room_id = game_host_.create_room(white_player, black_player, db_game_id);

    white->assign_room(room_id);
    black->assign_room(room_id);

    runtime_store_.register_room(room_id, static_cast<UserId>(white->player().id()),
                                 static_cast<UserId>(black->player().id()), server_id_);

    return room_id;
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
