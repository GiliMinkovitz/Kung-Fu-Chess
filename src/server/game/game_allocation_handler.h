#pragma once

#include "server/game/i_game_host.h"
#include "server/game/protocol/game_creation_request.h"
#include "server/game/protocol/game_creation_response.h"

#include <string>

namespace kfc {

class IRuntimeStore;

class GameAllocationHandler {
public:
    GameAllocationHandler(IGameHost& game_host, IRuntimeStore& runtime_store, std::string server_id,
                          std::string endpoint);

    [[nodiscard]] GameCreationResponse allocate(const GameCreationRequest& request);

private:
    IGameHost& game_host_;
    IRuntimeStore& runtime_store_;
    std::string server_id_;
    std::string endpoint_;
};

}  // namespace kfc
