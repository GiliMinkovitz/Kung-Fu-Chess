#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace kfc {

using Row = std::vector<std::string>;
using Board = std::vector<Row>;

enum class BoardError {
    Ok,
    UnknownToken,
    RowWidthMismatch,
};

struct VplInput {
    Board board;
    BoardError error = BoardError::Ok;
    std::vector<std::string> commands;
};

[[nodiscard]] bool is_valid_token(const std::string& token) noexcept;
[[nodiscard]] bool is_valid_board(const Board& board) noexcept;
[[nodiscard]] BoardError parse_board_rows(const std::vector<std::string>& lines, Board& board);
[[nodiscard]] const char* board_error_message(BoardError error) noexcept;

VplInput read_vpl_input(std::istream& in);
void write_board(std::ostream& out, const Board& board);

}  // namespace kfc
