#include "server/network/session_message_sink.h"

#include "server/client_connection.h"
#include "server/player_session.h"
#include "server/session/client_session_manager.h"

#include <string>

namespace kfc {

SessionMessageSink::SessionMessageSink(ClientSessionManager& session_manager)
    : session_manager_(session_manager) {}

PlayerSession* SessionMessageSink::find_session(PlayerId player) const {
    return session_manager_.find_session(player);
}

bool SessionMessageSink::send(PlayerId player, std::string_view message) {
    PlayerSession* session = find_session(player);
    if (session == nullptr) {
        return false;
    }

    ClientConnection* connection = session->connection();
    if (connection == nullptr) {
        return false;
    }

    return connection->try_send(std::string(message));
}

bool SessionMessageSink::send_message(PlayerId player, std::string_view message) {
    PlayerSession* session = find_session(player);
    if (session == nullptr) {
        return false;
    }

    ClientConnection* connection = session->connection();
    if (connection == nullptr) {
        return false;
    }

    return connection->send_message(std::string(message));
}

}  // namespace kfc
