#pragma once

#include "server/client_connection.h"

#include <boost/asio/ip/tcp.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace kfc {

class WebSocketServer {
public:
    static constexpr std::size_t kMaxClients = 2;

    explicit WebSocketServer(unsigned short port);

    void try_accept();
    void prune_disconnected();

    [[nodiscard]] std::vector<ClientConnection>& clients() noexcept {
        return clients_;
    }

    [[nodiscard]] const std::vector<ClientConnection>& clients() const noexcept {
        return clients_;
    }

    void broadcast(const std::string& message);

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::vector<ClientConnection> clients_;
};

}  // namespace kfc
