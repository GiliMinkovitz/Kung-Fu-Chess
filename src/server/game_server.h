#pragma once

#include "server/authentication_service.h"
#include "server/game_result_message_writer.h"
#include "server/game_room.h"
#include "server/matchmaking.h"
#include "server/player_session.h"
#include "server/rating_service.h"
#include "server/session_registry.h"
#include "server/websocket_server.h"

#include "database/game_repository.h"
#include "database/player_repository.h"
#include "database/sqlite_database.h"
#include "model/board_model.h"
#include "model/piece.h"

#include <chrono>
#include <cstddef>
#include <list>
#include <optional>
#include <string>

namespace kfc {

class GameServer {
public:
    GameServer(unsigned short port, BoardModel default_board, const std::string& db_path = "kfc.db");

    void run();
    void tick_once();
#ifdef KFC_TEST_BUILD
    void request_stop() noexcept { stop_requested_ = true; }
    void finish_active_room_for_tests(std::optional<PieceColor> winner_color, FinishReason reason);
#endif

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] Matchmaking& matchmaking() noexcept;
    [[nodiscard]] GameRoom& room() noexcept;
    [[nodiscard]] SqliteDatabase& database() noexcept;
    [[nodiscard]] PlayerRepository& player_repository() noexcept;
    [[nodiscard]] GameRepository& game_repository() noexcept;

private:
    void accept_new_clients();
    void process_pending_logins();
    void process_matchmaking_timeouts();
    void prune_sessions();
    void process_active_room(std::int64_t elapsed, std::chrono::steady_clock::time_point& last_tick);
    void process_room_player_messages(PlayerSession& session, Match& match);
    void probe_active_room_connections();
    [[nodiscard]] std::optional<PieceColor> disconnected_player_color() const;
    [[nodiscard]] bool both_room_players_disconnected() const;
    void finish_active_room(std::optional<PieceColor> winner_color, FinishReason reason);
    [[nodiscard]] const Player* find_player_by_color(PieceColor color) const;
    [[nodiscard]] std::optional<RatingChange> update_ratings_for_result(PieceColor winner_color,
                                                                        int game_id);
    void cleanup_finished_room();
    void refresh_session_player(PlayerSession& session);

    WebSocketServer websocket_server_;
    Matchmaking matchmaking_;
    GameRoom room_;
    SqliteDatabase database_;
    PlayerRepository player_repository_;
    GameRepository game_repository_;
    RatingService rating_service_;
    AuthenticationService authentication_service_;
    SessionRegistry session_registry_;
    std::list<PlayerSession> sessions_;
    std::size_t next_session_id_ = 0;
    std::chrono::steady_clock::time_point last_tick_{};
#ifdef KFC_TEST_BUILD
    bool stop_requested_ = false;
#endif
};

}  // namespace kfc
