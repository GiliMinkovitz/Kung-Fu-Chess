#pragma once

#include "server/game/i_game_host.h"

#include <string>

namespace kfc {

class LoopbackGameHost : public IGameHost {
public:
    LoopbackGameHost(std::string bind_address, unsigned short internal_port,
                     std::string game_server_id, std::string game_endpoint);

    GameCreationResponse create_room(const GameCreationRequest& request) override;

private:
    std::string bind_address_;
    unsigned short internal_port_;
    std::string game_server_id_;
    std::string game_endpoint_;
};

}  // namespace kfc
