#pragma once

#include "board_view_model.h"
#include "../logic/game_state.h"

namespace kfc {

class BoardViewBuilder {
public:
    static BoardViewModel build(const GameState& state);
};

}  // namespace kfc
