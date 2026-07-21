#include "server/websocket_server.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
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
    if (ws_) {
        return;
    }

    beast::error_code accept_ec;
    tcp::socket socket{acceptor_.get_executor()};
    acceptor_.accept(socket, accept_ec);

    if (!accept_ec) {
        ws_ = std::make_unique<websocket::stream<tcp::socket>>(std::move(socket));

        beast::error_code handshake_ec;
        ws_->accept(handshake_ec);
        if (handshake_ec) {
            ws_.reset();
            throw beast::system_error{handshake_ec};
        }

        beast::get_lowest_layer(*ws_).non_blocking(true);

        if (!client_connected_logged_) {
            std::cout << "Client connected\n";
            client_connected_logged_ = true;
        }
    } else if (accept_ec != net::error::would_block) {
        throw beast::system_error{accept_ec};
    }
}

std::optional<std::string> WebSocketServer::try_read() {
    if (!ws_) {
        return std::nullopt;
    }

    beast::flat_buffer buffer;
    beast::error_code read_ec;
    ws_->read(buffer, read_ec);

    if (read_ec == net::error::would_block) {
        return std::nullopt;
    }

    if (read_ec == websocket::error::closed) {
        if (!client_disconnected_logged_) {
            std::cout << "Client disconnected\n";
            client_disconnected_logged_ = true;
        }
        ws_.reset();
        return std::nullopt;
    }

    if (read_ec) {
        throw beast::system_error{read_ec};
    }

    return beast::buffers_to_string(buffer.data());
}

bool WebSocketServer::try_send(const std::string& message) {
    if (!ws_) {
        return false;
    }

    beast::error_code write_ec;
    ws_->write(net::buffer(message), write_ec);

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec) {
        throw beast::system_error{write_ec};
    }

    return true;
}

}  // namespace kfc
