#pragma once

#include "network/websocket_client.h"

#include <cstddef>

namespace kfc {

// Maps client UI intents to outgoing wire commands. Does not access GameState,
// validate moves, or simulate selection semantics.
class NetworkInputHandler {
public:
    explicit NetworkInputHandler(WebSocketClient& client);

    bool send_select(std::size_t row, std::size_t col);
    bool send_move(std::size_t row, std::size_t col);
    bool send_jump(std::size_t row, std::size_t col);
    bool send_clear();

private:
    WebSocketClient& client_;
};

}  // namespace kfc
