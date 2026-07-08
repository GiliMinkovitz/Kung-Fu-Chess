#pragma once

#include "board_model.h"

#include <iosfwd>

namespace kfc {

void write_board(std::ostream& out, const BoardModel& board);

}  // namespace kfc
