#include "server/player.h"

namespace kfc {

Player::Player(int id, std::string username, int rating)
    : id_(id), username_(std::move(username)), rating_(rating) {}

int Player::id() const noexcept {
    return id_;
}

const std::string& Player::username() const noexcept {
    return username_;
}

int Player::rating() const noexcept {
    return rating_;
}

}  // namespace kfc
