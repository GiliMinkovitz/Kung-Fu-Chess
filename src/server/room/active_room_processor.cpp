#include "server/room/active_room_processor.h"

#include "app/observability/metric_counters.h"
#include "model/game_config.h"
#include "server/game_message_parser.h"
#include "server/network/i_message_sink.h"
#include "server/room/game_player.h"
#include "server/room/room.h"

#include <string>

namespace kfc {

ActiveRoomProcessor::ActiveRoomProcessor(RoomManager& room_manager,
                                         GameInputDispatcher& input_dispatcher,
                                         IMessageSink& message_sink)
    : room_manager_(room_manager),
      input_dispatcher_(input_dispatcher),
      message_sink_(message_sink) {}

void ActiveRoomProcessor::process(std::int64_t elapsed,
                                  std::chrono::steady_clock::time_point& last_tick,
                                  FinishCallback finish_callback) {
    if (room_manager_.active_rooms().empty()) {
        return;
    }

    for (Room* room : room_manager_.active_rooms()) {
        const GamePlayer* white = room->white_player();
        const GamePlayer* black = room->black_player();
        if (white == nullptr || black == nullptr) {
            continue;
        }

        input_dispatcher_.probe_room(*room);

        if (input_dispatcher_.both_room_players_disconnected(*room)) {
            finish_callback(room->id(), std::nullopt, FinishReason::Disconnect);
            continue;
        }
        if (const std::optional<PieceColor> disconnected =
                input_dispatcher_.disconnected_player_color(*room)) {
            const PieceColor winner =
                *disconnected == PieceColor::White ? PieceColor::Black : PieceColor::White;
            finish_callback(room->id(), winner, FinishReason::Disconnect);
            continue;
        }

        if (elapsed >= kTargetFrameMs && !room->is_game_over()) {
            room->tick(elapsed);

            const std::string snapshot = room->generate_snapshot();
            if (input_dispatcher_.is_player_connected(white->player_id)) {
                message_sink_.send(white->player_id, snapshot);
                kfc::app::observability::metrics().snapshots_sent_total.fetch_add(
                    1, std::memory_order_relaxed);
            }
            if (input_dispatcher_.is_player_connected(black->player_id)) {
                message_sink_.send(black->player_id, snapshot);
                kfc::app::observability::metrics().snapshots_sent_total.fetch_add(
                    1, std::memory_order_relaxed);
            }

            last_tick = std::chrono::steady_clock::now();
        }

        if (room->active() && !room->is_game_over()) {
            apply_room_inputs(*room, input_dispatcher_.poll_room_inputs(*room), finish_callback);
        }

        if (room->is_game_over()) {
            finish_callback(room->id(), room->match().state().winning_color(),
                            FinishReason::KingCapture);
        }
    }
}

void ActiveRoomProcessor::apply_room_inputs(Room& room, const std::vector<RoomPlayerInput>& inputs,
                                            FinishCallback& finish_callback) {
    for (const RoomPlayerInput& input : inputs) {
        if (input.kind == RoomPlayerInput::Kind::Resign) {
            const PieceColor winner =
                input.player_side == PieceColor::White ? PieceColor::Black : PieceColor::White;
            finish_callback(room.id(), winner, FinishReason::Resign);
            return;
        }

        if (is_action_allowed(input.player_side, room.match(), input.action)) {
            room.submit_action(input.action);
            kfc::app::observability::metrics().player_actions_total.fetch_add(
                1, std::memory_order_relaxed);
        }
    }
}

}  // namespace kfc
