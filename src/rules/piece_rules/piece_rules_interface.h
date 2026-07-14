#pragma once

#include "../../model/board_model.h"

namespace kfc::piece_rules {

using MoveValidatorFn = bool (*)(const BoardModel&, int, int, int, int);

struct PieceRuleEntry {
    MoveValidatorFn validator = nullptr;
    bool requires_destination_check = false;
};

}  // namespace kfc::piece_rules
