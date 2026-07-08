#pragma once

#include "piece.h"

#include <cstddef>
#include <initializer_list>
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

    [[nodiscard]] Piece piece_at(std::size_t row, std::size_t col) const;
    [[nodiscard]] bool is_empty(std::size_t row, std::size_t col) const;
    void set_piece(std::size_t row, std::size_t col, Piece piece);

    [[nodiscard]] std::string token_at(std::size_t row, std::size_t col) const;

    void append_row(std::vector<Piece> row);

    static BoardModel from_token_grid(
        std::initializer_list<std::initializer_list<const char*>> rows);

private:
    std::vector<std::vector<Piece>> cells_;
};

[[nodiscard]] bool operator==(const BoardModel& lhs, const BoardModel& rhs) noexcept;

}  // namespace kfc
