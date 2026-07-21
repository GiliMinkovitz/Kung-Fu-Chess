#include "network/network_input_handler.h"

#include <string>

namespace kfc {

NetworkInputHandler::NetworkInputHandler(WebSocketClient& client) : client_{client} {}

bool NetworkInputHandler::send_select(std::size_t row, std::size_t col) {
    return client_.try_send("select " + std::to_string(row) + ' ' + std::to_string(col));
}

bool NetworkInputHandler::send_move(std::size_t row, std::size_t col) {
    return client_.try_send("move " + std::to_string(row) + ' ' + std::to_string(col));
}

bool NetworkInputHandler::send_jump(std::size_t row, std::size_t col) {
    return client_.try_send("jump " + std::to_string(row) + ' ' + std::to_string(col));
}

bool NetworkInputHandler::send_clear() {
    return client_.try_send("clear");
}

}  // namespace kfc
