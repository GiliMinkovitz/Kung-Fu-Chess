#include "core/board_model.h"

#include "core/game_config.h"
#include "core/piece_factory.h"
#include "core/piece_token.h"

#include <cassert>
#include <stdexcept>

namespace kfc {

namespace {

[[nodiscard]] Piece& piece_at_index(std::vector<Piece>& pieces, Piece::Id id) {
    if (id == Piece::kInvalidId || id > pieces.size()) {
        throw std::out_of_range("invalid piece id");
    }
    return pieces[id - 1];
}

[[nodiscard]] const Piece& piece_at_index(const std::vector<Piece>& pieces, Piece::Id id) {
    if (id == Piece::kInvalidId || id > pieces.size()) {
        throw std::out_of_range("invalid piece id");
    }
    return pieces[id - 1];
}

}  // namespace

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

    for (const std::vector<std::optional<Piece::Id>>& row : cells_) {
        if (row.size() != width) {
            return false;
        }
    }

    return true;
}

bool BoardModel::is_empty(std::size_t row, std::size_t col) const {
    return !cells_[row][col].has_value();
}

const Piece* BoardModel::piece_at(std::size_t row, std::size_t col) const {
    const std::optional<Piece::Id>& id = cells_[row][col];
    if (!id.has_value()) {
        return nullptr;
    }
    return find_piece_by_id(*id);
}

Piece* BoardModel::piece_at(std::size_t row, std::size_t col) {
    const std::optional<Piece::Id>& id = cells_[row][col];
    if (!id.has_value()) {
        return nullptr;
    }
    return find_piece_by_id(*id);
}

const Piece& BoardModel::get_piece(Piece::Id id) const {
    return piece_at_index(pieces_, id);
}

Piece& BoardModel::get_piece(Piece::Id id) {
    return piece_at_index(pieces_, id);
}

void BoardModel::store_piece(Piece piece) {
    if (piece.id == Piece::kInvalidId || piece.id > pieces_.size() + 1) {
        throw std::invalid_argument("piece id must be assigned sequentially by PieceFactory");
    }
    if (piece.id == pieces_.size() + 1) {
        pieces_.push_back(std::move(piece));
        return;
    }
    pieces_[piece.id - 1] = std::move(piece);
}

void BoardModel::place_piece(Piece piece) {
    const auto row = static_cast<std::size_t>(piece.cell.row);
    const auto col = static_cast<std::size_t>(piece.cell.col);
    place_piece_at(row, col, std::move(piece));
}

void BoardModel::place_piece_at(std::size_t row, std::size_t col, Piece piece) {
    const Piece::Id piece_id = piece.id;
    piece.cell = Position{static_cast<int>(row), static_cast<int>(col)};
    store_piece(std::move(piece));
    cells_[row][col] = piece_id;
}

void BoardModel::remove_piece_at(std::size_t row, std::size_t col) {
    const std::optional<Piece::Id>& id = cells_[row][col];
    if (id.has_value()) {
        get_piece(*id).state = PieceState::Captured;
    }
    cells_[row][col].reset();
}

void BoardModel::clear_cell(std::size_t row, std::size_t col) {
    cells_[row][col].reset();
}

std::string BoardModel::token_at(std::size_t row, std::size_t col) const {
    const Piece* piece = piece_at(row, col);
    if (piece == nullptr) {
        return std::string{kEmptyToken};
    }
    return to_token(piece->color, piece->kind);
}

void BoardModel::append_token_row(const std::vector<std::string>& tokens, std::size_t row_index,
                                  PieceFactory& factory) {
    std::vector<std::optional<Piece::Id>> cell_row;
    cell_row.reserve(tokens.size());

    std::size_t col_index = 0;
    for (const std::string& token : tokens) {
        if (is_empty_token(token)) {
            cell_row.push_back(std::nullopt);
        } else {
            const std::optional<PieceDescriptor> descriptor = descriptor_from_token(token);
            assert(descriptor.has_value());
            Piece piece = factory.create(
                descriptor->color, descriptor->kind,
                Position{static_cast<int>(row_index), static_cast<int>(col_index)});
            store_piece(std::move(piece));
            cell_row.push_back(pieces_.back().id);
        }
        ++col_index;
    }

    cells_.push_back(std::move(cell_row));
}

BoardModel BoardModel::from_token_grid(
    std::initializer_list<std::initializer_list<const char*>> rows) {
    BoardModel board;
    PieceFactory factory;

    std::size_t row_index = 0;
    for (const std::initializer_list<const char*>& row : rows) {
        std::vector<std::optional<Piece::Id>> cell_row;
        cell_row.reserve(row.size());

        std::size_t col_index = 0;
        for (const char* token_cstr : row) {
            const std::string token(token_cstr);
            if (is_empty_token(token)) {
                cell_row.push_back(std::nullopt);
            } else {
                const std::optional<PieceDescriptor> descriptor = descriptor_from_token(token);
                assert(descriptor.has_value());
                Piece piece = factory.create(
                    descriptor->color, descriptor->kind,
                    Position{static_cast<int>(row_index), static_cast<int>(col_index)});
                const Piece::Id piece_id = piece.id;
                board.store_piece(std::move(piece));
                cell_row.push_back(piece_id);
            }
            ++col_index;
        }

        board.cells_.push_back(std::move(cell_row));
        ++row_index;
    }

    return board;
}

Piece* BoardModel::find_piece_by_id(Piece::Id id) {
    if (id == Piece::kInvalidId || id > pieces_.size()) {
        return nullptr;
    }
    return &pieces_[id - 1];
}

const Piece* BoardModel::find_piece_by_id(Piece::Id id) const {
    if (id == Piece::kInvalidId || id > pieces_.size()) {
        return nullptr;
    }
    return &pieces_[id - 1];
}

bool operator==(const BoardModel& lhs, const BoardModel& rhs) noexcept {
    if (lhs.rows() != rhs.rows() || lhs.cols() != rhs.cols()) {
        return false;
    }
    for (std::size_t row = 0; row < lhs.rows(); ++row) {
        for (std::size_t col = 0; col < lhs.cols(); ++col) {
            if (lhs.token_at(row, col) != rhs.token_at(row, col)) {
                return false;
            }
        }
    }
    return true;
}

}  // namespace kfc
