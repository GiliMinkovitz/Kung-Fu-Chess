#include "server/player_session.h"

#include <stdexcept>

namespace kfc {

PlayerSession::PlayerSession(std::size_t id,
                             ClientConnection* connection,
                             PlayerRepository& player_repository)
    : id_(id), player_(load_or_create_player(player_repository)), connection_(connection) {}

std::string PlayerSession::next_username() {
    static int next = 1;
    return "Player" + std::to_string(next++);
}

Player PlayerSession::load_or_create_player(PlayerRepository& player_repository) {
    const std::string username = next_username();

    if (const auto existing = player_repository.find_by_username(username)) {
        return *existing;
    }

    if (const auto created = player_repository.create_player(username, 1000)) {
        return *created;
    }

    throw std::runtime_error("Failed to load or create player");
}

}  // namespace kfc
