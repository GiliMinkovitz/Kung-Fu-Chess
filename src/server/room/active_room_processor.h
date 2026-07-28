#pragma once

#include "model/piece.h"
#include "server/game_result_message_writer.h"
#include "server/room/room_manager.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

namespace kfc {

class PlayerSession;
class Room;

class ActiveRoomProcessor {
public:
    using FinishCallback =
        std::function<void(RoomId, std::optional<PieceColor>, FinishReason)>;

    explicit ActiveRoomProcessor(RoomManager& room_manager);

    void process(std::int64_t delta_ms, std::chrono::steady_clock::time_point& last_tick,
                  FinishCallback finish_callback);

private:
    void probe_room_connections(Room& room);
    [[nodiscard]] std::optional<PieceColor> disconnected_player_color(const Room& room) const;
    [[nodiscard]] bool both_room_players_disconnected(const Room& room) const;
    void process_room_player_messages(Room& room, PlayerSession& session,
                                      FinishCallback& finish_callback);
    void process_room_player_messages_if_playing(Room& room, PlayerSession* session,
                                                 FinishCallback& finish_callback);

    RoomManager& room_manager_;
};

}  // namespace kfc
