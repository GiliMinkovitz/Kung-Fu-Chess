#pragma once

#include "../rules/board_validator.h"
#include "../model/board_model.h"

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
