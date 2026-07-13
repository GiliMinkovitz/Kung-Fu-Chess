#include "move_validator.h"

#include "rules_registry.h"

namespace kfc {

namespace {

[[nodiscard]] bool is_destination_legal(const BoardModel& board, int start_row, int start_col,
                                        int end_row, int end_col) {
    const Piece* dest =
        board.piece_at(static_cast<std::size_t>(end_row), static_cast<std::size_t>(end_col));
    if (dest == nullptr) {
        return true;
    }
    const Piece* moving = board.piece_at(static_cast<std::size_t>(start_row),
                                         static_cast<std::size_t>(start_col));
    if (moving == nullptr) {
        return false;
    }
    return moving->is_opponent_of(*dest);
}

}  // namespace

bool is_legal_move(const BoardModel& board, PieceKind kind, int start_row, int start_col,
                   int end_row, int end_col) {
    if (board.rows() == 0 || board.cols() == 0) {
        return false;
    }
    if (!board.contains(start_row, start_col) || !board.contains(end_row, end_col)) {
        return false;
    }
    if (start_row == end_row && start_col == end_col) {
        return false;
    }

    const piece_rules::PieceRuleEntry rule = piece_rules::get_rule_entry(kind);
    if (rule.validator == nullptr) {
        return false;
    }
    if (!rule.validator(board, start_row, start_col, end_row, end_col)) {
        return false;
    }
    if (rule.requires_destination_check) {
        return is_destination_legal(board, start_row, start_col, end_row, end_col);
    }
    return true;
}

}  // namespace kfc
