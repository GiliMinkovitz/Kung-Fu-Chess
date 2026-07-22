#include "server/game_room.h"

namespace kfc {

GameRoom::GameRoom(BoardModel board, std::vector<PlayerSession*> players)
    : match_(std::move(board)), players_(std::move(players)) {}

Match& GameRoom::match() noexcept {
    return match_;
}

const Match& GameRoom::match() const noexcept {
    return match_;
}

const std::vector<PlayerSession*>& GameRoom::players() const noexcept {
    return players_;
}

}  // namespace kfc
