#pragma once

#include "server/session/i_session_disconnect_handler.h"

namespace kfc {

class MatchmakingService;

class MonolithMatchmakingDisconnectHandler final : public ISessionDisconnectHandler {
public:
    explicit MonolithMatchmakingDisconnectHandler(MatchmakingService& matchmaking_service);

    void on_session_disconnected(PlayerSession& session) override;

private:
    MatchmakingService& matchmaking_service_;
};

}  // namespace kfc
