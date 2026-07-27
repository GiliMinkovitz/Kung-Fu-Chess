#include "server/player_session.h"

namespace kfc {

PlayerSession::PlayerSession(std::size_t id, ClientConnection* connection)
    : id_(id), connection_(connection) {}

void PlayerSession::assign_user(const UserId user_id, const std::string& username, int rating) {
    user_id_ = user_id;
    player_profile_ = Player(static_cast<int>(user_id), username, rating);
}

void PlayerSession::bind_player(Player player) {
    assign_user(static_cast<UserId>(player.id()), player.username(), player.rating());
}

void PlayerSession::clear_user() {
    user_id_.reset();
    player_profile_.reset();
}

}  // namespace kfc
