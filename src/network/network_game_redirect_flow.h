#pragma once

#include "network/game_redirect_info.h"

namespace kfc {

class NetworkInputHandler;
class WebSocketClient;

class NetworkGameRedirectFlow {
public:
    NetworkGameRedirectFlow(WebSocketClient& lobby_client, WebSocketClient& game_client,
                            NetworkInputHandler& game_input);

    bool execute(const GameRedirectInfo& redirect);

    [[nodiscard]] bool lobby_closed() const noexcept { return lobby_closed_; }
    [[nodiscard]] bool game_connected() const noexcept { return game_connected_; }
    [[nodiscard]] bool join_game_sent() const noexcept { return join_game_sent_; }

private:
    WebSocketClient& lobby_client_;
    WebSocketClient& game_client_;
    NetworkInputHandler& game_input_;
    bool lobby_closed_ = false;
    bool game_connected_ = false;
    bool join_game_sent_ = false;
};

}  // namespace kfc
