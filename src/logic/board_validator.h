#pragma once

#include "core/board_model.h"

#include <string>
#include <vector>

namespace kfc {

enum class BoardError {
    Ok,
    UnknownToken,
    RowWidthMismatch,
};

[[nodiscard]] BoardError parse_board_rows(const std::vector<std::string>& lines, BoardModel& board);
[[nodiscard]] const char* board_error_message(BoardError error) noexcept;

}  // namespace kfc
