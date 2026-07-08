#include "board_model.h"

#include <cassert>

namespace kfc {

bool BoardModel::is_in_bounds(std::size_t row, std::size_t col) const noexcept {
    if (cells_.empty()) {
        return false;
    }
    return row < cells_.size() && col < cells_.front().size();
}

bool BoardModel::contains(int row, int col) const noexcept {
    if (row < 0 || col < 0) {
        return false;
    }
    return is_in_bounds(static_cast<std::size_t>(row), static_cast<std::size_t>(col));
}

bool BoardModel::is_valid() const noexcept {
    if (cells_.empty()) {
        return false;
    }

    const std::size_t width = cells_.front().size();
    if (width == 0) {
        return false;
    }

    for (const std::vector<Piece>& row : cells_) {
        if (row.size() != width) {
            return false;
        }
    }

    return true;
}

Piece BoardModel::piece_at(std::size_t row, std::size_t col) const {
    return cells_[row][col];
}

bool BoardModel::is_empty(std::size_t row, std::size_t col) const {
    return cells_[row][col].is_empty();
}

void BoardModel::set_piece(std::size_t row, std::size_t col, Piece piece) {
    cells_[row][col] = piece;
}

std::string BoardModel::token_at(std::size_t row, std::size_t col) const {
    return cells_[row][col].to_token();
}

void BoardModel::append_row(std::vector<Piece> row) {
    cells_.push_back(std::move(row));
}

BoardModel BoardModel::from_token_grid(
    std::initializer_list<std::initializer_list<const char*>> rows) {
    BoardModel board;
    for (const std::initializer_list<const char*>& row : rows) {
        std::vector<Piece> pieces;
        pieces.reserve(row.size());
        for (const char* token : row) {
            const std::optional<Piece> piece = Piece::from_token(token);
            assert(piece.has_value());
            pieces.push_back(*piece);
        }
        board.cells_.push_back(std::move(pieces));
    }
    return board;
}

bool operator==(const BoardModel& lhs, const BoardModel& rhs) noexcept {
    if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
        return false;
    }
    for (std::size_t row = 0; row < lhs.rows(); ++row) {
        for (std::size_t col = 0; col < lhs.cols(); ++col) {
            if (lhs.piece_at(row, col) != rhs.piece_at(row, col)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace kfc
