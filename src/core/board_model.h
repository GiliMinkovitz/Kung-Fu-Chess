#pragma once

#include "piece.h"
#include "piece_factory.h"

#include <cstddef>
#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

namespace kfc {

class BoardModel {
public:
    [[nodiscard]] std::size_t rows() const noexcept { return cells_.size(); }
    [[nodiscard]] std::size_t cols() const noexcept {
        return cells_.empty() ? 0 : cells_.front().size();
    }
    [[nodiscard]] bool is_in_bounds(std::size_t row, std::size_t col) const noexcept;
    [[nodiscard]] bool contains(int row, int col) const noexcept;
    [[nodiscard]] bool is_valid() const noexcept;

    [[nodiscard]] bool is_empty(std::size_t row, std::size_t col) const;
    [[nodiscard]] const Piece* piece_at(std::size_t row, std::size_t col) const;
    [[nodiscard]] Piece* piece_at(std::size_t row, std::size_t col);

    [[nodiscard]] Piece& get_piece(Piece::Id id);

    void place_piece(Piece piece);
    void place_piece_at(std::size_t row, std::size_t col, Piece piece);
    void remove_piece_at(std::size_t row, std::size_t col);
    void clear_cell(std::size_t row, std::size_t col);

    [[nodiscard]] std::string token_at(std::size_t row, std::size_t col) const;

    [[nodiscard]] Piece::Id next_piece_id() const noexcept {
        return static_cast<Piece::Id>(pieces_.size() + 1);
    }

    void append_token_row(const std::vector<std::string>& tokens, std::size_t row_index,
                          PieceFactory& factory);

    static BoardModel from_token_grid(
        std::initializer_list<std::initializer_list<const char*>> rows);

private:
    void store_piece(Piece piece);
    [[nodiscard]] Piece* find_piece_by_id(Piece::Id id);
    [[nodiscard]] const Piece* find_piece_by_id(Piece::Id id) const;

    std::vector<std::vector<std::optional<Piece::Id>>> cells_;
    std::vector<Piece> pieces_;
};

[[nodiscard]] bool operator==(const BoardModel& lhs, const BoardModel& rhs) noexcept;

}  // namespace kfc
