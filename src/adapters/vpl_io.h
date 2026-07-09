#pragma once

#include "logic/board_validator.h"
#include "core/board_model.h"

#include <iosfwd>
#include <string>
#include <vector>

namespace kfc {

struct VplInput {
    BoardModel board;
    BoardError error = BoardError::Ok;
    std::vector<std::string> commands;
};

VplInput read_vpl_input(std::istream& in);

}  // namespace kfc
