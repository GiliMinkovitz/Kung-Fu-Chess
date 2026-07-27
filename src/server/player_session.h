#pragma once

#include "model/piece.h"
#include "server/client_connection.h"
#include "server/player.h"
#include "server/room/room.h"
#include "server/user/user_id.h"

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
    [[nodiscard]] bool has_user() const noexcept;
    [[nodiscard]] bool has_player() const noexcept;
    void assign_user(UserId user_id, const std::string& username, int rating);
    void bind_player(Player player);
    void clear_user();
    [[nodiscard]] UserId user_id() const;
    [[nodiscard]] int rating() const noexcept;
    void request_play();
    void cancel_search();
    void set_playing();
    void set_side(PieceColor side);
    void clear_side();
    [[nodiscard]] bool has_side() const noexcept;
    [[nodiscard]] PieceColor side() const;
    void assign_room(RoomId room_id);
    void clear_room();
    [[nodiscard]] bool has_room() const noexcept;
    [[nodiscard]] RoomId room_id() const;
    [[nodiscard]] Player& player() noexcept;
    [[nodiscard]] const Player& player() const noexcept;
    [[nodiscard]] ClientConnection* connection() noexcept;
    [[nodiscard]] const ClientConnection* connection() const noexcept;

private:
    std::size_t id_;
    PlayerSessionState state_ = PlayerSessionState::Connected;
    std::optional<UserId> user_id_;
    std::optional<Player> player_profile_;
    std::optional<PieceColor> side_;
    std::optional<RoomId> room_id_;
    ClientConnection* connection_;
};

inline std::size_t PlayerSession::id() const noexcept {
    return id_;
}

inline PlayerSessionState PlayerSession::state() const noexcept {
    return state_;
}

inline bool PlayerSession::has_user() const noexcept {
    return user_id_.has_value();
}

inline bool PlayerSession::has_player() const noexcept {
    return has_user();
}

inline UserId PlayerSession::user_id() const {
    return *user_id_;
}

inline int PlayerSession::rating() const noexcept {
    return player_profile_->rating();
}

inline void PlayerSession::request_play() {
    if (state_ == PlayerSessionState::Connected && has_user()) {
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

inline void PlayerSession::assign_room(RoomId room_id) {
    room_id_ = room_id;
}

inline void PlayerSession::clear_room() {
    room_id_.reset();
}

inline bool PlayerSession::has_room() const noexcept {
    return room_id_.has_value();
}

inline RoomId PlayerSession::room_id() const {
    return *room_id_;
}

inline Player& PlayerSession::player() noexcept {
    return *player_profile_;
}

inline const Player& PlayerSession::player() const noexcept {
    return *player_profile_;
}

inline ClientConnection* PlayerSession::connection() noexcept {
    return connection_;
}

inline const ClientConnection* PlayerSession::connection() const noexcept {
    return connection_;
}

}  // namespace kfc
