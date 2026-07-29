#pragma once

#include "server/game/i_game_allocator.h"

namespace kfc {

class IGameHost;

class LocalGameAllocator : public IGameAllocator {
public:
    explicit LocalGameAllocator(IGameHost& game_host);

    GameCreationResponse allocate_game(const GameCreationRequest& request) override;

private:
    IGameHost& game_host_;
};

}  // namespace kfc
