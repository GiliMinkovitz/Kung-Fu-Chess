#include "server/websocket_server.h"

#include <iostream>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

WebSocketServer::WebSocketServer(unsigned short port)
    : acceptor_{
          io_context_,
          tcp::endpoint{net::ip::make_address("127.0.0.1"), port},
      } {
    acceptor_.non_blocking(true);
}

void WebSocketServer::try_accept() {
    if (clients_.size() >= kMaxClients) {
        return;
    }

    beast::error_code accept_ec;
    tcp::socket socket{acceptor_.get_executor()};
    acceptor_.accept(socket, accept_ec);

    if (!accept_ec) {
        clients_.emplace_back(std::move(socket));
        std::cout << "Client connected (" << clients_.size() << "/"
                  << kMaxClients << ")\n";
    } else if (accept_ec != net::error::would_block) {
        throw beast::system_error{accept_ec};
    }
}

void WebSocketServer::prune_disconnected() {
    const std::size_t before = clients_.size();

    std::vector<ClientConnection> kept;
    kept.reserve(clients_.size());
    for (ClientConnection& client : clients_) {
        if (client.is_open()) {
            kept.emplace_back(std::move(client));
        }
    }
    clients_ = std::move(kept);

    if (clients_.size() < before) {
        std::cout << "Client disconnected (" << clients_.size() << "/"
                  << kMaxClients << ")\n";
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    for (ClientConnection& client : clients_) {
        client.try_send(message);
    }
}

}  // namespace kfc
