#include "network/network_game_redirect_flow.h"

#include "network/network_input_handler.h"
#include "network/websocket_client.h"

namespace kfc {

NetworkGameRedirectFlow::NetworkGameRedirectFlow(WebSocketClient& lobby_client,
                                                 WebSocketClient& game_client,
                                                 NetworkInputHandler& game_input)
    : lobby_client_{lobby_client},
      game_client_{game_client},
      game_input_{game_input} {}

bool NetworkGameRedirectFlow::execute(const GameRedirectInfo& redirect) {
    if (lobby_client_.is_connected()) {
        lobby_client_.disconnect();
    }
    lobby_closed_ = true;

    game_client_.connect_to_game_server(redirect.endpoint);
    game_connected_ = game_client_.is_connected();
    if (!game_connected_) {
        return false;
    }

    join_game_sent_ = game_input_.send_join_game(redirect.room_id);
    return join_game_sent_;
}

}  // namespace kfc
