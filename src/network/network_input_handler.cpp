#include "network/network_input_handler.h"

#include <string>

namespace kfc {

NetworkInputHandler::NetworkInputHandler(WebSocketClient& client) : client_{client} {}

bool NetworkInputHandler::send_login(const std::string& username, const std::string& password) {
    if (password.empty()) {
        return client_.try_send("login " + username);
    }
    return client_.try_send("login " + username + " " + password);
}

bool NetworkInputHandler::send_play() {
    return client_.try_send("play");
}

bool NetworkInputHandler::send_join_game(const RoomId room_id) {
    return client_.try_send("join_game " + std::to_string(room_id));
}

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

bool NetworkInputHandler::send_resign() {
    return client_.try_send("resign");
}

}  // namespace kfc
