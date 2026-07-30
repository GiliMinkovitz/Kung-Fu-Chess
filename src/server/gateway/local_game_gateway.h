#pragma once

#include "server/gateway/i_game_gateway.h"

namespace kfc {

class IMessageSink;

class LocalGameGateway : public IGameGateway {
public:
    explicit LocalGameGateway(IMessageSink& message_sink);

    void notify_match_found(PlayerId white, PlayerId black) override;
    void notify_game_start(PlayerId white, PlayerId black) override;
    void notify_search_timeout(PlayerId player) override;
    void send_game_redirect(PlayerId player, GatewayGameRedirectInfo redirect_info) override;

private:
    IMessageSink& message_sink_;
};

}  // namespace kfc
