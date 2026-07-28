#include "server/websocket_server.h"

#include "app/runtime_diagnostics.h"

#include <iostream>

namespace beast = boost::beast;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

WebSocketServer::WebSocketServer(unsigned short port)
    : WebSocketServer([port] {
          app::ServerConfig config;
          config.port = port;
          return config;
      }()) {}

WebSocketServer::WebSocketServer(const app::ServerConfig& server_config)
    : acceptor_{
          io_context_,
          tcp::endpoint{net::ip::make_address(server_config.bind_address), server_config.port},
      },
      max_clients_{server_config.max_clients} {
    acceptor_.set_option(net::socket_base::reuse_address{true});
    acceptor_.non_blocking(true);
}

unsigned short WebSocketServer::port() const {
    return acceptor_.local_endpoint().port();
}

void WebSocketServer::try_accept() {
    if (clients_.size() >= max_clients_) {
        return;
    }

    beast::error_code accept_ec;
    tcp::socket socket{acceptor_.get_executor()};
    acceptor_.accept(socket, accept_ec);

    if (!accept_ec) {
        try {
            clients_.emplace_back(std::move(socket));
#ifndef KFC_TEST_BUILD
            if (app::diagnostics_enabled()) {
                std::cerr << "[DIAG] WebSocketServer::try_accept() handshake succeeded\n";
                std::cout << "Client connected (" << clients_.size() << "/"
                          << max_clients_ << ")\n";
            }
#endif
        } catch (const std::exception& ex) {
#ifndef KFC_TEST_BUILD
            if (app::diagnostics_enabled()) {
                std::cerr << "[DIAG] WebSocketServer::try_accept() handshake failed: "
                          << ex.what() << '\n';
            }
#endif
            throw;
        }
    } else if (accept_ec != net::error::would_block) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cerr << "[DIAG] WebSocketServer::try_accept() accept failed: "
                      << accept_ec.message() << '\n';
        }
#endif
        throw beast::system_error{accept_ec};
    }
}

void WebSocketServer::prune_disconnected() {
    const std::size_t before = clients_.size();

    for (auto it = clients_.begin(); it != clients_.end();) {
        it->probe_disconnect();
        if (!it->is_open()) {
            it = clients_.erase(it);
        } else {
            ++it;
        }
    }

    if (clients_.size() < before) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cout << "Client disconnected (" << clients_.size() << "/"
                      << max_clients_ << ")\n";
        }
#endif
    }
}

void WebSocketServer::broadcast(const std::string& message) {
    for (ClientConnection& client : clients_) {
        client.try_send(message);
    }
}

#ifdef KFC_TEST_BUILD
void WebSocketServer::close_acceptor_for_tests() {
    beast::error_code close_ec;
    acceptor_.close(close_ec);
}
#endif

}  // namespace kfc
