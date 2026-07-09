#include "logic/game_state.h"

#include "core/piece_factory.h"

#include <algorithm>

namespace kfc {

namespace {

[[nodiscard]] bool can_settle_move(const BoardModel& board, const GameRules& rules,
                                   const PendingMove& move) {
    const auto [start_row, start_col] = move.start_pos;
    const auto [end_row, end_col] = move.end_pos;

    const Piece* piece = board.piece_at(start_row, start_col);
    if (piece == nullptr || piece->id != move.piece_id) {
        return false;
    }

    return rules.is_legal_move(board, piece->kind, static_cast<int>(start_row),
                               static_cast<int>(start_col), static_cast<int>(end_row),
                               static_cast<int>(end_col));
}

}  // namespace

GameState::GameState(BoardModel board) : GameState(std::move(board), KungFuChessRules::standard()) {}

GameState::GameState(BoardModel board, GameRules rules)
    : board_(std::move(board)), rules_(std::move(rules)) {}

const Piece* GameState::piece_at(std::size_t row, std::size_t col) const {
    return board_.piece_at(row, col);
}

std::string GameState::token_at(std::size_t row, std::size_t col) const {
    return board_.token_at(row, col);
}

bool GameState::is_empty(std::size_t row, std::size_t col) const {
    return board_.is_empty(row, col);
}

void GameState::place_piece_at(std::size_t row, std::size_t col, Piece piece) {
    board_.place_piece_at(row, col, std::move(piece));
}

void GameState::place_new_piece_at(std::size_t row, std::size_t col, PieceColor color,
                                   PieceKind kind) {
    PieceFactory factory(board_.next_piece_id());
    board_.place_piece_at(row, col,
                          factory.create(color, kind,
                                         Position{static_cast<int>(row), static_cast<int>(col)}));
}

bool GameState::same_board_layout_as(const GameState& other) const noexcept {
    return board_ == other.board_;
}

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
    return scheduler_.is_piece_moving(row, col);
}

bool GameState::is_piece_jumping(std::size_t row, std::size_t col) const {
    return scheduler_.is_piece_jumping(row, col);
}

bool GameState::is_selectable_piece(std::size_t row, std::size_t col) const {
    return is_piece(row, col) && !is_piece_moving(row, col) && !is_piece_jumping(row, col);
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
    return rules_.is_legal_move(board_, moving->kind, start_row, start_col, end_row, end_col);
}

void GameState::add_clock(std::int64_t ms) {
    scheduler_.add_clock(ms);
    settle_pending_moves();
}

void GameState::settle_pending_moves() {
    scheduler_.for_each_pending_due([this](const PendingMove& move) {
        if (!can_settle_move(board_, rules_, move)) {
            return;
        }

        const auto [start_row, start_col] = move.start_pos;
        const auto [end_row, end_col] = move.end_pos;

        board_.clear_cell(start_row, start_col);

        const ArrivingPieceInfo arriving_piece_info{
            move.piece_id,
            move.start_pos,
            move.end_pos,
        };

        if (!scheduler_.check_for_jump_capture(collision_resolver_, board_, rules_, move.end_pos,
                                               arriving_piece_info, game_over_)) {
            Piece updated = board_.get_piece(move.piece_id);
            updated = rules_.on_reach_last_row(updated, end_row, board_.rows());
            updated.cell = Position{static_cast<int>(end_row), static_cast<int>(end_col)};
            updated.state = PieceState::Idle;
            board_.place_piece(std::move(updated));
        } else {
            Piece& arriving_piece = board_.get_piece(move.piece_id);
            arriving_piece.state = PieceState::Captured;
        }
    });

    scheduler_.expire_jumps();
}

void GameState::select(std::size_t row, std::size_t col) {
    if (!is_in_bounds(row, col)) {
        return;
    }
    selected_ = std::make_pair(row, col);
}

void GameState::clear_selection() {
    selected_.reset();
}

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
    if (scheduler_.is_same_color_destination_claimed(moving->color, {to_row, to_col})) {
        return false;
    }

    const Piece* destination = board_.piece_at(to_row, to_col);
    const bool is_capture = destination != nullptr && destination->is_opponent_of(*moving);
    const std::int64_t move_duration =
        compute_move_duration(from_row, from_col, to_row, to_col, is_capture);

    const PendingMove proposed{
        moving->id,
        moving->color,
        {from_row, from_col},
        {to_row, to_col},
        scheduler_.clock_ms() + move_duration,
    };
    return !scheduler_.conflicts_with_opposite_color_move(moving->color, proposed);
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

    scheduler_.schedule_move(PendingMove{
        moving->id,
        moving->color,
        {from_row, from_col},
        {to_row, to_col},
        scheduler_.clock_ms() + move_duration,
    });
    clear_selection();
}

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

    scheduler_.schedule_jump(JumpState{
        piece->id,
        piece->color,
        {row, col},
        scheduler_.clock_ms() + rules_.jump_duration_ms,
    });
}

void GameState::write_board(
    std::ostream& out, const std::function<void(std::ostream&, const BoardModel&)>& writer) {
    settle_pending_moves();
    writer(out, board_);
}

}  // namespace kfc
