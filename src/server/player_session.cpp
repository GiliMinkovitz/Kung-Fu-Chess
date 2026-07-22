#include "server/player_session.h"

#include <stdexcept>

namespace kfc {

PlayerSession::PlayerSession(std::size_t id, ClientConnection* connection)
    : id_(id), connection_(connection) {}

bool PlayerSession::login(const std::string& username, PlayerRepository& player_repository) {
    if (has_player() || username.empty()) {
        return false;
    }

    player_ = load_or_create_player(username, player_repository);
    return true;
}

Player PlayerSession::load_or_create_player(const std::string& username,
                                            PlayerRepository& player_repository) {
    if (const auto existing = player_repository.find_by_username(username)) {
        return *existing;
    }

    if (const auto created = player_repository.create_player(username, 1000)) {
        return *created;
    }

    throw std::runtime_error("Failed to load or create player");
}

}  // namespace kfc
