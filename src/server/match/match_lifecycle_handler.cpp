#include "server/match/match_lifecycle_handler.h"

#include "app/i_runtime_store.h"
#include "app/runtime_diagnostics.h"
#include "database/i_game_repository.h"
#include "model/piece.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/network/i_message_sink.h"
#include "server/network/player_id.h"
#include "server/game/i_game_host.h"
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

void MatchLifecycleHandler::bind_message_sink(IMessageSink& message_sink) {
    message_sink_ = &message_sink;
}

RoomId MatchLifecycleHandler::create_match(PlayerSession* white, PlayerSession* black) {
    white->set_playing();
    black->set_playing();
    white->set_side(PieceColor::White);
    black->set_side(PieceColor::Black);

    const RoomId room_id = game_host_.create_room();

    game_host_.activate_room(room_id, to_game_player(*white, PieceColor::White),
                             to_game_player(*black, PieceColor::Black));

    const std::optional<int> db_game_id =
        game_repository_.create_game(white->player().id(), black->player().id());
    if (db_game_id.has_value()) {
        game_host_.set_db_game_id(room_id, *db_game_id);
    }

    white->assign_room(room_id);
    black->assign_room(room_id);

    runtime_store_.register_room(room_id, static_cast<UserId>(white->player().id()),
                                 static_cast<UserId>(black->player().id()), server_id_);

    return room_id;
}

void MatchLifecycleHandler::notify_match_created(const MatchCreated& match) {
    if (message_sink_ == nullptr) {
        return;
    }

    message_sink_->send(static_cast<PlayerId>(match.white->id()), "match_found white");
    message_sink_->send(static_cast<PlayerId>(match.black->id()), "match_found black");
    message_sink_->send(static_cast<PlayerId>(match.white->id()), "game_start white");
    message_sink_->send(static_cast<PlayerId>(match.black->id()), "game_start black");
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
        if (message_sink_ != nullptr) {
            message_sink_->send(static_cast<PlayerId>(session->id()), "search_timeout");
        }
        session->cancel_search();
    }
}

}  // namespace kfc
