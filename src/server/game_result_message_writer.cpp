#include "server/game_result_message_writer.h"

namespace kfc {

namespace {

const char* finish_reason_token(FinishReason reason) {
    switch (reason) {
        case FinishReason::KingCapture:
            return "checkmate";
        case FinishReason::Resign:
            return "resign";
        case FinishReason::Disconnect:
            return "opponent_disconnect";
    }
    return "unknown";
}

}  // namespace

std::string create_game_result_message(bool won, FinishReason reason, int rating) {
    return std::string("game_result ") + (won ? "win" : "loss") + ' ' + finish_reason_token(reason) +
           ' ' + std::to_string(rating) + '\n';
}

}  // namespace kfc
