#pragma once

#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"

namespace kfc {

class IGameAllocator {
public:
    virtual ~IGameAllocator() = default;

    virtual GameCreationResponse allocate_game(const GameCreationRequest& request) = 0;
};

}  // namespace kfc
