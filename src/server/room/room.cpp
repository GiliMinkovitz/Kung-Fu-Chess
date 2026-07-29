#include "server/room/room.h"

#include "server/snapshot_writer.h"
#include "ui/view/board_view_builder.h"

namespace kfc {

Room::Room(RoomId id, BoardModel board)
    : id_(id), default_board_(board), match_(std::move(board)) {}

RoomId Room::id() const noexcept {
    return id_;
}

bool Room::active() const noexcept {
    return active_;
}

void Room::activate(const GamePlayer& white, const GamePlayer& black) {
    white_player_ = white;
    black_player_ = black;
    active_ = true;
}

void Room::rebind_player(const UserId user_id, const PlayerId new_player_id) {
    if (white_player_.has_value() && white_player_->user_id == user_id) {
        white_player_->player_id = new_player_id;
    }
    if (black_player_.has_value() && black_player_->user_id == user_id) {
        black_player_->player_id = new_player_id;
    }
}

void Room::set_db_game_id(int id) {
    db_game_id_ = id;
}

void Room::reset() {
    active_ = false;
    white_player_.reset();
    black_player_.reset();
    db_game_id_.reset();
    match_ = Match(default_board_);
}

void Room::tick(std::int64_t delta_ms) {
    match_.tick(delta_ms);
}

void Room::submit_action(const GameAction& action) {
    match_.submit_action(action);
}

std::string Room::generate_snapshot() const {
    const BoardViewModel view = BoardViewBuilder::build(match_.state());
    return write_snapshot(view);
}

bool Room::is_game_over() const noexcept {
    return match_.is_game_over();
}

bool Room::contains_player(const UserId user_id) const noexcept {
    return (white_player_.has_value() && white_player_->user_id == user_id) ||
           (black_player_.has_value() && black_player_->user_id == user_id);
}

const GamePlayer* Room::white_player() const noexcept {
    return white_player_ ? &*white_player_ : nullptr;
}

const GamePlayer* Room::black_player() const noexcept {
    return black_player_ ? &*black_player_ : nullptr;
}

std::optional<int> Room::db_game_id() const noexcept {
    return db_game_id_;
}

Match& Room::match() noexcept {
    return match_;
}

const Match& Room::match() const noexcept {
    return match_;
}

}  // namespace kfc
