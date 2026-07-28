#include "server/match/match_lifecycle_handler.h"

#include "app/i_runtime_store.h"
#include "app/runtime_diagnostics.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/player_session.h"
#include "server/room/room.h"
#include "server/room/room_manager.h"
#include "server/user/user_id.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <utility>

namespace kfc {

MatchLifecycleHandler::MatchLifecycleHandler(RoomManager& room_manager,
                                             IGameRepository& game_repository,
                                             IRuntimeStore& runtime_store, std::string server_id)
    : room_manager_(room_manager),
      game_repository_(game_repository),
      runtime_store_(runtime_store),
      server_id_(std::move(server_id)) {}

void MatchLifecycleHandler::bind_matchmaking_service(MatchmakingService& matchmaking_service) {
    matchmaking_service_ = &matchmaking_service;
}

RoomId MatchLifecycleHandler::create_match(PlayerSession* white, PlayerSession* black) {
    white->set_playing();
    black->set_playing();
    white->set_side(PieceColor::White);
    black->set_side(PieceColor::Black);

    const RoomId room_id = room_manager_.create_room();

    Room* room = room_manager_.find_room(room_id);
    room->activate(&white->player(), &black->player());

    const std::optional<int> db_game_id =
        game_repository_.create_game(white->player().id(), black->player().id());
    room->bind_sessions(white, black);
    if (db_game_id.has_value()) {
        room->set_db_game_id(*db_game_id);
    }

    white->assign_room(room_id);
    black->assign_room(room_id);

    runtime_store_.register_room(room_id, static_cast<UserId>(white->player().id()),
                                 static_cast<UserId>(black->player().id()), server_id_);

    return room_id;
}

void MatchLifecycleHandler::notify_match_created(const MatchCreated& match) {
    match.white->connection()->try_send("match_found white");
    match.black->connection()->try_send("match_found black");
    match.white->connection()->try_send("game_start white");
    match.black->connection()->try_send("game_start black");
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
        session->connection()->try_send("search_timeout");
        session->cancel_search();
    }
}

}  // namespace kfc
