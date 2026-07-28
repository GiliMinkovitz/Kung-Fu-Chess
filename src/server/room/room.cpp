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

void Room::activate(Player* white, Player* black) {
    white_player_ = white;
    black_player_ = black;
    active_ = true;
}

void Room::bind_sessions(PlayerSession* white, PlayerSession* black) {
    white_session_ = white;
    black_session_ = black;
}

void Room::set_db_game_id(int id) {
    db_game_id_ = id;
}

void Room::reset() {
    active_ = false;
    white_player_ = nullptr;
    black_player_ = nullptr;
    white_session_ = nullptr;
    black_session_ = nullptr;
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

bool Room::contains_player(const Player* player) const noexcept {
    if (player == nullptr) {
        return false;
    }
    return player == white_player_ || player == black_player_;
}

Player* Room::white_player() noexcept {
    return white_player_;
}

Player* Room::black_player() noexcept {
    return black_player_;
}

const Player* Room::white_player() const noexcept {
    return white_player_;
}

const Player* Room::black_player() const noexcept {
    return black_player_;
}

PlayerSession* Room::white_session() noexcept {
    return white_session_;
}

PlayerSession* Room::black_session() noexcept {
    return black_session_;
}

const PlayerSession* Room::white_session() const noexcept {
    return white_session_;
}

const PlayerSession* Room::black_session() const noexcept {
    return black_session_;
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
