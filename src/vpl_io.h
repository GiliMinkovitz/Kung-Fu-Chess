#pragma once

#include "board_model.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace kfc {

enum class BoardError {
    Ok,
    UnknownToken,
    RowWidthMismatch,
};

struct VplInput {
    BoardModel board;
    BoardError error = BoardError::Ok;
    std::vector<std::string> commands;
};

[[nodiscard]] BoardError parse_board_rows(const std::vector<std::string>& lines, BoardModel& board);
[[nodiscard]] const char* board_error_message(BoardError error) noexcept;

VplInput read_vpl_input(std::istream& in);
void write_board(std::ostream& out, const BoardModel& board);

}  // namespace kfc
