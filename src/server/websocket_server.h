#pragma once

#include "server/client_connection.h"

#include <boost/asio/ip/tcp.hpp>

#include <cstddef>
#include <list>
#include <string>

namespace kfc {

class WebSocketServer {
public:
    static constexpr std::size_t kMaxClients = 2;

    explicit WebSocketServer(unsigned short port);

    [[nodiscard]] unsigned short port() const;

    void try_accept();
    void prune_disconnected();

    [[nodiscard]] std::list<ClientConnection>& clients() noexcept {
        return clients_;
    }

    [[nodiscard]] const std::list<ClientConnection>& clients() const noexcept {
        return clients_;
    }

    void broadcast(const std::string& message);

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::list<ClientConnection> clients_;
};

}  // namespace kfc
