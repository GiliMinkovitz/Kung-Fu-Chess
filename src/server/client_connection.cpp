#include "server/client_connection.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace kfc {

ClientConnection::ClientConnection(net::ip::tcp::socket socket)
    : ws_{std::move(socket)} {
    beast::error_code handshake_ec;
    ws_.accept(handshake_ec);
    if (handshake_ec) {
        open_ = false;
        throw beast::system_error{handshake_ec};
    }
}

std::optional<std::string> ClientConnection::try_read() {
    if (!open_) {
        return std::nullopt;
    }

    beast::error_code avail_ec;
    if (beast::get_lowest_layer(ws_).available(avail_ec) == 0) {
        return std::nullopt;
    }
    if (avail_ec) {
        std::cerr << "[SERVER-CONN-DIAG] try_read() available error: "
                  << avail_ec.message() << " (" << avail_ec.value() << ")\n";
        open_ = false;
        return std::nullopt;
    }

    beast::flat_buffer buffer;
    beast::error_code read_ec;
    ws_.read(buffer, read_ec);

    if (read_ec == net::error::would_block) {
        return std::nullopt;
    }

    if (read_ec == websocket::error::closed) {
        open_ = false;
        return std::nullopt;
    }

    if (read_ec) {
        std::cerr << "[SERVER-CONN-DIAG] try_read() websocket error: "
                  << read_ec.message() << " (" << read_ec.value() << ")\n";
        open_ = false;
        return std::nullopt;
    }

    return beast::buffers_to_string(buffer.data());
}

bool ClientConnection::try_send(const std::string& message) {
    if (!open_) {
        return false;
    }

    beast::error_code write_ec;
    ws_.write(net::buffer(message), write_ec);

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec == websocket::error::closed) {
        std::cerr << "[SERVER-CONN-DIAG] try_send() websocket error: "
                  << write_ec.message() << " (" << write_ec.value() << ")\n";
        open_ = false;
        return false;
    }

    if (write_ec) {
        std::cerr << "[SERVER-CONN-DIAG] try_send() websocket error: "
                  << write_ec.message() << " (" << write_ec.value() << ")\n";
        open_ = false;
        return false;
    }

    return true;
}

}  // namespace kfc
