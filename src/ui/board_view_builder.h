#pragma once

#include "i_ui_renderer.h"
#include "../logic/game_state.h"

namespace kfc {

class BoardViewBuilder {
public:
    static BoardViewModel build(const GameState& state);
};

}  // namespace kfc
