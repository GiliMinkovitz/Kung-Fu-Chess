#pragma once

#include "server/gateway/game_redirect_info.h"
#include "server/network/player_id.h"

namespace kfc {

class IGameGateway {
public:
    virtual ~IGameGateway() = default;

    virtual void notify_match_found(PlayerId white, PlayerId black) = 0;
    virtual void notify_game_start(PlayerId white, PlayerId black) = 0;
    virtual void notify_search_timeout(PlayerId player) = 0;
    virtual void send_game_redirect(PlayerId player, GatewayGameRedirectInfo redirect_info) = 0;
};

}  // namespace kfc
