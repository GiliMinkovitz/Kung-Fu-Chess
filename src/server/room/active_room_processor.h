#pragma once

#include "model/piece.h"
#include "server/game_result_message_writer.h"
#include "server/network/game_input_dispatcher.h"
#include "server/room/room_manager.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

namespace kfc {

class IMessageSink;
class Room;

class ActiveRoomProcessor {
public:
    using FinishCallback =
        std::function<void(RoomId, std::optional<PieceColor>, FinishReason)>;

    ActiveRoomProcessor(RoomManager& room_manager, GameInputDispatcher& input_dispatcher,
                        IMessageSink& message_sink);

    void process(std::int64_t delta_ms, std::chrono::steady_clock::time_point& last_tick,
                  FinishCallback finish_callback);

private:
    void apply_room_inputs(Room& room, const std::vector<RoomPlayerInput>& inputs,
                           FinishCallback& finish_callback);

    RoomManager& room_manager_;
    GameInputDispatcher& input_dispatcher_;
    IMessageSink& message_sink_;
};

}  // namespace kfc
