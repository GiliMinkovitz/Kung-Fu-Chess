#include "server/game_room.h"

namespace kfc {

GameRoom::GameRoom(BoardModel board)
    : default_board_(board), match_(std::move(board)) {}

bool GameRoom::active() const noexcept {
    return active_;
}

void GameRoom::activate(PlayerSession* white, PlayerSession* black) {
    white_player_ = white;
    black_player_ = black;
    active_ = true;
}

void GameRoom::reset() {
    active_ = false;
    white_player_ = nullptr;
    black_player_ = nullptr;
    match_ = Match(default_board_);
}

bool GameRoom::contains(const PlayerSession* session) const noexcept {
    return session == white_player_ || session == black_player_;
}

PlayerSession* GameRoom::white_player() noexcept {
    return white_player_;
}

PlayerSession* GameRoom::black_player() noexcept {
    return black_player_;
}

const PlayerSession* GameRoom::white_player() const noexcept {
    return white_player_;
}

const PlayerSession* GameRoom::black_player() const noexcept {
    return black_player_;
}

Match& GameRoom::match() noexcept {
    return match_;
}

const Match& GameRoom::match() const noexcept {
    return match_;
}

}  // namespace kfc
