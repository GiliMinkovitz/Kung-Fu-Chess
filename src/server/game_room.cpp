#include "server/game_room.h"

namespace kfc {

GameRoom::GameRoom(BoardModel board)
    : default_board_(board), match_(std::move(board)) {}

bool GameRoom::active() const noexcept {
    return active_;
}

void GameRoom::activate(PlayerSession* white, PlayerSession* black,
                        GameRepository& game_repository) {
    white_player_ = white;
    black_player_ = black;
    active_ = true;
    db_game_id_ = game_repository.create_game(white->player().id(), black->player().id());
}

void GameRoom::reset() {
    active_ = false;
    white_player_ = nullptr;
    black_player_ = nullptr;
    db_game_id_.reset();
    match_ = Match(default_board_);
}

std::optional<int> GameRoom::db_game_id() const noexcept {
    return db_game_id_;
}

bool GameRoom::contains(const PlayerSession* session) const noexcept {
    return session == white_player_ || session == black_player_;
}

PlayerSession* GameRoom::white_session() noexcept {
    return white_player_;
}

PlayerSession* GameRoom::black_session() noexcept {
    return black_player_;
}

const PlayerSession* GameRoom::white_session() const noexcept {
    return white_player_;
}

const PlayerSession* GameRoom::black_session() const noexcept {
    return black_player_;
}

Player* GameRoom::white_player() noexcept {
    return white_player_ != nullptr ? &white_player_->player() : nullptr;
}

Player* GameRoom::black_player() noexcept {
    return black_player_ != nullptr ? &black_player_->player() : nullptr;
}

const Player* GameRoom::white_player() const noexcept {
    return white_player_ != nullptr ? &white_player_->player() : nullptr;
}

const Player* GameRoom::black_player() const noexcept {
    return black_player_ != nullptr ? &black_player_->player() : nullptr;
}

Match& GameRoom::match() noexcept {
    return match_;
}

const Match& GameRoom::match() const noexcept {
    return match_;
}

}  // namespace kfc
