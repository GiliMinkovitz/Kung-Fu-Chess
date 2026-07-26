#include "server/player_session.h"

namespace kfc {

PlayerSession::PlayerSession(std::size_t id, ClientConnection* connection)
    : id_(id), connection_(connection) {}

void PlayerSession::bind_player(Player player) {
    player_ = std::move(player);
}

}  // namespace kfc
