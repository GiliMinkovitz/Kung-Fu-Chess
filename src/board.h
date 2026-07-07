#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace kfc {

using Board = std::vector<std::string>;

[[nodiscard]] bool is_valid_board(const Board& board) noexcept;

void read_board(std::istream& in, Board& board);
void write_board(std::ostream& out, const Board& board);

}  // namespace kfc
