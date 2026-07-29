#pragma once

namespace kfc {

class PlayerSession;

class ISessionDisconnectHandler {
public:
    virtual ~ISessionDisconnectHandler() = default;

    virtual void on_session_disconnected(PlayerSession& session) = 0;
};

}  // namespace kfc
