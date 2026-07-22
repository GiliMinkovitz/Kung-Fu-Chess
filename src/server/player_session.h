#pragma once

#include "database/player_repository.h"
#include "model/piece.h"
#include "server/client_connection.h"
#include "server/player.h"

#include <cstddef>
#include <optional>
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
    [[nodiscard]] bool has_player() const noexcept;
    bool login(const std::string& username, PlayerRepository& player_repository);
    void request_play();
    void cancel_search();
    void set_playing();
    void set_side(PieceColor side);
    void clear_side();
    [[nodiscard]] bool has_side() const noexcept;
    [[nodiscard]] PieceColor side() const;
    [[nodiscard]] Player& player() noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] ClientConnection* connection() noexcept;
    [[nodiscard]] const ClientConnection* connection() const noexcept;

private:
    static Player load_or_create_player(const std::string& username,
                                        PlayerRepository& player_repository);

    std::size_t id_;
    PlayerSessionState state_ = PlayerSessionState::Connected;
    std::optional<Player> player_;
    std::optional<PieceColor> side_;
    ClientConnection* connection_;
};

inline std::size_t PlayerSession::id() const noexcept {
    return id_;
}

inline PlayerSessionState PlayerSession::state() const noexcept {
    return state_;
}

inline bool PlayerSession::has_player() const noexcept {
    return player_.has_value();
}

inline void PlayerSession::request_play() {
    if (state_ == PlayerSessionState::Connected && has_player()) {
        state_ = PlayerSessionState::Searching;
    }
}

inline void PlayerSession::cancel_search() {
    if (state_ == PlayerSessionState::Searching) {
        state_ = PlayerSessionState::Connected;
    }
}

inline void PlayerSession::set_playing() {
    state_ = PlayerSessionState::Playing;
}

inline void PlayerSession::set_side(PieceColor side) {
    side_ = side;
}

inline void PlayerSession::clear_side() {
    side_.reset();
}

inline bool PlayerSession::has_side() const noexcept {
    return side_.has_value();
}

inline PieceColor PlayerSession::side() const {
    return *side_;
}

inline Player& PlayerSession::player() noexcept {
    return *player_;
}

inline const Player& PlayerSession::player() const noexcept {
    return *player_;
}

inline ClientConnection* PlayerSession::connection() noexcept {
    return connection_;
}

inline const ClientConnection* PlayerSession::connection() const noexcept {
    return connection_;
}

}  // namespace kfc
