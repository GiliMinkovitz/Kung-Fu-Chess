#pragma once

#include "server/client_connection.h"

#include <cstddef>

namespace kfc {

class PlayerSession {
public:
    PlayerSession(std::size_t id, ClientConnection* connection);

    [[nodiscard]] std::size_t id() const noexcept;
    [[nodiscard]] ClientConnection* connection() noexcept;
    [[nodiscard]] const ClientConnection* connection() const noexcept;

private:
    std::size_t id_;
    ClientConnection* connection_;
};

inline PlayerSession::PlayerSession(std::size_t id, ClientConnection* connection)
    : id_(id), connection_(connection) {}

inline std::size_t PlayerSession::id() const noexcept {
    return id_;
}

inline ClientConnection* PlayerSession::connection() noexcept {
    return connection_;
}

inline const ClientConnection* PlayerSession::connection() const noexcept {
    return connection_;
}

}  // namespace kfc
