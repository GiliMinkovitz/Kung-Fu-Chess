#pragma once

#include "server/network/i_message_sink.h"

namespace kfc {

class ClientSessionManager;

class SessionMessageSink : public IMessageSink {
public:
    explicit SessionMessageSink(ClientSessionManager& session_manager);

    bool send(PlayerId player, std::string_view message) override;
    bool send_message(PlayerId player, std::string_view message) override;

private:
    [[nodiscard]] class PlayerSession* find_session(PlayerId player) const;

    ClientSessionManager& session_manager_;
};

}  // namespace kfc
