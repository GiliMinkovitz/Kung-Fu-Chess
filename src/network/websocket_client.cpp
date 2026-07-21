#include "network/websocket_client.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

WebSocketClient::WebSocketClient(std::string host, std::uint16_t port)
    : host_{std::move(host)}, port_{port} {}

void WebSocketClient::connect() {
    if (connected_) {
        return;
    }

    try {
        tcp::resolver resolver{io_context_};
        const tcp::resolver::results_type endpoints =
            resolver.resolve(host_, std::to_string(port_));

        tcp::socket socket{io_context_};
        beast::error_code connect_ec;
        net::connect(socket, endpoints, connect_ec);
        if (connect_ec) {
            throw beast::system_error{connect_ec};
        }

        ws_.emplace(std::move(socket));
        ws_->set_option(
            websocket::stream_base::timeout::suggested(beast::role_type::client));

        beast::error_code handshake_ec;
        ws_->handshake(host_, "/", handshake_ec);
        if (handshake_ec) {
            ws_.reset();
            throw beast::system_error{handshake_ec};
        }

        connected_ = true;
        std::cerr << "[DIAG] WebSocketClient::connect() succeeded\n";
    } catch (const std::exception& ex) {
        std::cerr << "[DIAG] WebSocketClient::connect() failed: " << ex.what()
                  << '\n';
        throw;
    }
}

void WebSocketClient::disconnect() {
    if (!connected_ || !ws_) {
        connected_ = false;
        return;
    }

    beast::error_code close_ec;
    ws_->close(websocket::close_code::normal, close_ec);
    ws_.reset();
    connected_ = false;
}

bool WebSocketClient::try_send(const std::string& message) {
    if (!connected_ || !ws_) {
        return false;
    }

    beast::error_code write_ec;
    ws_->write(net::buffer(message), write_ec);

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec == websocket::error::closed || write_ec == net::error::eof) {
        connected_ = false;
        return false;
    }

    if (write_ec) {
        connected_ = false;
        throw beast::system_error{write_ec};
    }

    return true;
}

std::optional<std::string> WebSocketClient::try_receive_snapshot() {
    if (!connected_ || !ws_) {
        return std::nullopt;
    }

    beast::error_code avail_ec;
    if (beast::get_lowest_layer(*ws_).available(avail_ec) == 0) {
        return std::nullopt;
    }
    if (avail_ec) {
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() available error: "
                  << avail_ec.message() << " (" << avail_ec.value() << ")\n";
        connected_ = false;
        return std::nullopt;
    }

    beast::flat_buffer buffer;
    beast::error_code read_ec;
    ws_->read(buffer, read_ec);

    if (read_ec == net::error::would_block) {
        return std::nullopt;
    }

    if (read_ec == websocket::error::closed || read_ec == net::error::eof) {
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() websocket closed/error: "
                  << read_ec.message() << " (" << read_ec.value() << ")\n";
        connected_ = false;
        return std::nullopt;
    }

    if (read_ec) {
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() websocket read error: "
                  << read_ec.message() << " (" << read_ec.value() << ")\n";
        connected_ = false;
        throw beast::system_error{read_ec};
    }

    return beast::buffers_to_string(buffer.data());
}

}  // namespace kfc
