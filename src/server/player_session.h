#pragma once

#include "server/client_connection.h"
#include "server/player.h"

#include <cstddef>
#include <string>

namespace kfc {

enum class PlayerSessionState {
    Connected,
    Searching,
    Playing,
};

class PlayerSession {
public:
    PlayerSession(std::size_t id, ClientConnection* connection);

    [[nodiscard]] std::size_t id() const noexcept;
    [[nodiscard]] PlayerSessionState state() const noexcept;
    void request_play();
    void set_playing();
    [[nodiscard]] Player& player() noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] ClientConnection* connection() noexcept;
    [[nodiscard]] const ClientConnection* connection() const noexcept;

private:
    static int next_player_id();
    static Player create_player();

    std::size_t id_;
    PlayerSessionState state_ = PlayerSessionState::Connected;
    Player player_;
    ClientConnection* connection_;
};

inline Player PlayerSession::create_player() {
    const int player_id = next_player_id();
    return Player(player_id, "Player" + std::to_string(player_id), 1000);
}

inline PlayerSession::PlayerSession(std::size_t id, ClientConnection* connection)
    : id_(id), player_(create_player()), connection_(connection) {}

inline int PlayerSession::next_player_id() {
    static int next = 1;
    return next++;
}

inline std::size_t PlayerSession::id() const noexcept {
    return id_;
}

inline PlayerSessionState PlayerSession::state() const noexcept {
    return state_;
}

inline void PlayerSession::request_play() {
    if (state_ == PlayerSessionState::Connected) {
        state_ = PlayerSessionState::Searching;
    }
}

inline void PlayerSession::set_playing() {
    state_ = PlayerSessionState::Playing;
}

inline Player& PlayerSession::player() noexcept {
    return player_;
}

inline const Player& PlayerSession::player() const noexcept {
    return player_;
}

inline ClientConnection* PlayerSession::connection() noexcept {
    return connection_;
}

inline const ClientConnection* PlayerSession::connection() const noexcept {
    return connection_;
}

}  // namespace kfc
