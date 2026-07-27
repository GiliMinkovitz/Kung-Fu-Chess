#pragma once

#include "server/authentication_service.h"
#include "server/game_result_message_writer.h"
#include "server/matchmaking/match_created_handler.h"
#include "server/matchmaking/matchmaking_service.h"
#include "server/player_session.h"
#include "server/rating_service.h"
#include "server/room/room_manager.h"
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
#include <unordered_map>

namespace kfc {

class GameServer : public IMatchCreatedHandler {
public:
    GameServer(unsigned short port, BoardModel default_board, const std::string& db_path = "kfc.db");

    void run();
    void tick_once();
#ifdef KFC_TEST_BUILD
    void request_stop() noexcept { stop_requested_ = true; }
    void finish_active_room_for_tests(std::optional<PieceColor> winner_color, FinishReason reason);
#endif

    [[nodiscard]] WebSocketServer& websocket_server() noexcept;
    [[nodiscard]] MatchmakingService& matchmaking_service() noexcept;
    [[nodiscard]] RoomManager& room_manager() noexcept;
    [[nodiscard]] Room& room() noexcept;
    [[nodiscard]] std::optional<int> room_db_game_id() const noexcept;
    [[nodiscard]] SqliteDatabase& database() noexcept;
    [[nodiscard]] PlayerRepository& player_repository() noexcept;
    [[nodiscard]] GameRepository& game_repository() noexcept;

    RoomId create_match(PlayerSession* white, PlayerSession* black) override;

private:
    struct RoomContext {
        PlayerSession* white_session = nullptr;
        PlayerSession* black_session = nullptr;
        std::optional<int> db_game_id;
    };

    void accept_new_clients();
    void process_pending_logins();
    void process_matchmaking_timeouts();
    void notify_match_created(const MatchCreated& match);
    void prune_sessions();
    void process_active_rooms(std::int64_t elapsed, std::chrono::steady_clock::time_point& last_tick);
    void process_playing_session_messages();
    void process_room_player_messages(Room& room, PlayerSession& session);
    void probe_room_connections(const RoomContext& context);
    [[nodiscard]] std::optional<PieceColor> disconnected_player_color(const RoomContext& context) const;
    [[nodiscard]] bool both_room_players_disconnected(const RoomContext& context) const;
    void finish_room(RoomId room_id, std::optional<PieceColor> winner_color, FinishReason reason);
    [[nodiscard]] const Player* find_player_by_color(const Room& room, PieceColor color) const;
    [[nodiscard]] std::optional<RatingChange> update_ratings_for_result(const Room& room,
                                                                        PieceColor winner_color,
                                                                        int game_id);
    void cleanup_finished_room(RoomId room_id);
    void refresh_session_player(PlayerSession& session);
    [[nodiscard]] RoomContext* find_context(RoomId room_id);
    [[nodiscard]] const RoomContext* find_context(RoomId room_id) const;
    [[nodiscard]] Room* find_session_room(const PlayerSession& session);

    WebSocketServer websocket_server_;
    MatchmakingService matchmaking_service_;
    RoomManager room_manager_;
    std::optional<RoomId> last_room_id_;
    std::unordered_map<RoomId, RoomContext> room_contexts_;
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
