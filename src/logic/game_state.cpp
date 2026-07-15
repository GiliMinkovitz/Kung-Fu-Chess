#include "game_state.h"

#include <algorithm>

namespace kfc {

GameState::GameState(BoardModel board) : GameState(std::move(board), KungFuChessRules::standard()) {}

GameState::GameState(BoardModel board, GameRules rules)
    : board_(std::move(board)), rules_(std::move(rules)) {}

std::string GameState::token_at(std::size_t row, std::size_t col) const {
    return board_.token_at(row, col);
}

bool GameState::same_board_layout_as(const GameState& other) const noexcept {
    return board_ == other.board_;
}

// --- Selection & piece queries ---
bool GameState::selection(std::size_t& row, std::size_t& col) const {
    if (!selected_) {
        return false;
    }
    row = selected_->first;
    col = selected_->second;
    return true;
}

bool GameState::is_in_bounds(std::size_t row, std::size_t col) const noexcept {
    return board_.is_in_bounds(row, col);
}

bool GameState::is_piece(std::size_t row, std::size_t col) const {
    if (!is_in_bounds(row, col)) {
        return false;
    }
    return !board_.is_empty(row, col);
}

bool GameState::is_piece_moving(std::size_t row, std::size_t col) const {
    return arbiter_.is_piece_moving(row, col);
}

bool GameState::is_piece_jumping(std::size_t row, std::size_t col) const {
    return arbiter_.is_piece_jumping(row, col);
}

bool GameState::is_selectable_piece(std::size_t row, std::size_t col) const {
    return is_piece(row, col) && !is_piece_moving(row, col) && !is_piece_jumping(row, col);
}

AnimationSnapshot GameState::animations_for_render() const {
    return arbiter_.animations_for_render();
}

bool GameState::is_friendly_to_selection(std::size_t row, std::size_t col) const {
    if (!selected_ || !is_selectable_piece(row, col)) {
        return false;
    }

    const Piece* selected_piece = board_.piece_at(selected_->first, selected_->second);
    const Piece* cell_piece = board_.piece_at(row, col);
    if (selected_piece == nullptr || cell_piece == nullptr) {
        return false;
    }
    return cell_piece->is_same_color_as(*selected_piece);
}

bool GameState::is_legal_move(int start_row, int start_col, int end_row, int end_col) const {
    const Piece* moving =
        board_.piece_at(static_cast<std::size_t>(start_row), static_cast<std::size_t>(start_col));
    if (moving == nullptr) {
        return false;
    }
    // Routed through GameRules so callers share the same injectable policy as settlement.
    return rules_.is_legal_move(board_, moving->kind, start_row, start_col, end_row, end_col);
}

// --- Time / arbiter ---

void GameState::add_clock(std::int64_t ms) {
    arbiter_.update_time(ms, board_, rules_, game_over_);
}

void GameState::settle_pending_moves() {
    arbiter_.settle_pending_moves(board_, rules_, game_over_);
}

// --- Selection actions ---

void GameState::select(std::size_t row, std::size_t col) {
    if (!is_in_bounds(row, col)) {
        return;
    }
    selected_ = std::make_pair(row, col);
}

void GameState::clear_selection() {
    selected_.reset();
}

// --- Move logic ---

// Kung Fu Chess timing: captures use a fixed duration; non-captures scale with Chebyshev distance.
std::int64_t GameState::compute_move_duration(std::size_t from_row, std::size_t from_col,
                                              std::size_t to_row, std::size_t to_col,
                                              bool is_capture) const noexcept {
    if (is_capture) {
        return rules_.move_duration_ms;
    }

    const std::size_t row_delta = from_row > to_row ? from_row - to_row : to_row - from_row;
    const std::size_t col_delta = from_col > to_col ? from_col - to_col : to_col - from_col;
    return static_cast<std::int64_t>(std::max(row_delta, col_delta)) * rules_.move_duration_ms;
}

bool GameState::can_move_selected_to(std::size_t from_row, std::size_t from_col,
                                     std::size_t to_row, std::size_t to_col) const {
    if (!is_in_bounds(to_row, to_col)) {
        return false;
    }
    if (is_piece_moving(from_row, from_col) || is_piece_jumping(from_row, from_col)) {
        return false;
    }

    const Piece* moving = board_.piece_at(from_row, from_col);
    if (moving == nullptr) {
        return false;
    }
    if (!is_legal_move(static_cast<int>(from_row), static_cast<int>(from_col),
                       static_cast<int>(to_row), static_cast<int>(to_col))) {
        return false;
    }
    // Static legality alone is insufficient: two realtime constraints must also pass.
    if (arbiter_.is_same_color_destination_claimed(moving->color, {to_row, to_col})) {
        return false;
    }

    const Piece* destination = board_.piece_at(to_row, to_col);
    const bool is_capture = destination != nullptr && destination->is_opponent_of(*moving);
    const std::int64_t move_duration =
        compute_move_duration(from_row, from_col, to_row, to_col, is_capture);

    // Opposite-color moves sharing a route cannot both be in flight (Kung Fu Chess rule).
    return !arbiter_.would_conflict_with_opposite_color_move(
        moving->color, moving->id, {from_row, from_col}, {to_row, to_col}, move_duration);
}

void GameState::move_selected_to(std::size_t to_row, std::size_t to_col) {
    if (!selected_) {
        return;
    }

    const auto [from_row, from_col] = *selected_;
    if (!can_move_selected_to(from_row, from_col, to_row, to_col)) {
        return;
    }

    const Piece* moving = board_.piece_at(from_row, from_col);
    const Piece* destination = board_.piece_at(to_row, to_col);
    const bool is_capture = destination != nullptr && destination->is_opponent_of(*moving);
    const std::int64_t move_duration =
        compute_move_duration(from_row, from_col, to_row, to_col, is_capture);

    board_.get_piece(moving->id).state = PieceState::Moving;

    arbiter_.request_move(moving->id, moving->color, {from_row, from_col}, {to_row, to_col},
                          move_duration);
    clear_selection();
}

// --- Jump logic ---

void GameState::jump_selected() {
    if (!selected_) {
        return;
    }

    const auto [row, col] = *selected_;
    jump_at(row, col);
    clear_selection();
}

void GameState::jump_at(std::size_t row, std::size_t col) {
    if (!is_in_bounds(row, col) || board_.is_empty(row, col)) {
        return;
    }
    if (is_piece_moving(row, col) || is_piece_jumping(row, col)) {
        return;
    }

    const Piece* piece = board_.piece_at(row, col);
    if (piece == nullptr) {
        return;
    }

    arbiter_.request_jump(piece->id, piece->color, {row, col}, rules_.jump_duration_ms);
}

// --- Output ---

void GameState::write_board(
    std::ostream& out, const std::function<void(std::ostream&, const BoardModel&)>& writer) {
    settle_pending_moves();
    writer(out, board_);
}

}  // namespace kfc
