#include "server/game/local_game_allocator.h"

#include "server/game/i_game_host.h"

namespace kfc {

LocalGameAllocator::LocalGameAllocator(IGameHost& game_host) : game_host_(game_host) {}

GameCreationResponse LocalGameAllocator::allocate_game(const GameCreationRequest& request) {
    return game_host_.create_room(request);
}

}  // namespace kfc
