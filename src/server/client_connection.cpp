#include "server/client_connection.h"

#ifdef KFC_TEST_BUILD
#include "test/socket_test_hooks.h"
#endif

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace kfc {

bool ClientConnection::detect_peer_disconnect() {
    if (!open_) {
        return true;
    }

    auto& socket = beast::get_lowest_layer(ws_);

    beast::error_code avail_ec;
    std::size_t available_bytes = 0;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::next_avail_error) {
        avail_ec = *test::SocketTestHooks::next_avail_error;
        test::SocketTestHooks::next_avail_error.reset();
    } else
#endif
    {
        available_bytes = socket.available(avail_ec);
    }

    if (avail_ec) {
        open_ = false;
        return true;
    }

    if (available_bytes > 0) {
        return false;
    }

    const bool was_non_blocking = socket.non_blocking();
    if (!was_non_blocking) {
        beast::error_code nb_ec;
#ifdef KFC_TEST_BUILD
        if (test::SocketTestHooks::force_non_blocking_error) {
            nb_ec = net::error::operation_not_supported;
            test::SocketTestHooks::force_non_blocking_error = false;
        } else
#endif
        {
            socket.non_blocking(true, nb_ec);
        }
        if (nb_ec) {
            open_ = false;
            return true;
        }
    }

    beast::error_code peek_ec;
    char byte = 0;
    std::size_t peeked = 0;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::force_peek_success_with_data) {
        peek_ec = {};
        peeked = test::SocketTestHooks::forced_peeked.value_or(1);
        test::SocketTestHooks::force_peek_success_with_data = false;
        test::SocketTestHooks::forced_peeked.reset();
    } else if (test::SocketTestHooks::next_peek_error) {
        peek_ec = *test::SocketTestHooks::next_peek_error;
        peeked = test::SocketTestHooks::forced_peeked.value_or(0);
        test::SocketTestHooks::next_peek_error.reset();
        test::SocketTestHooks::forced_peeked.reset();
    } else
#endif
    {
        peeked = socket.receive(
            net::buffer(&byte, 1), net::socket_base::message_peek, peek_ec);
    }

    if (!was_non_blocking) {
        beast::error_code blocking_ec;
        socket.non_blocking(false, blocking_ec);
    }

    if (peek_ec == net::error::would_block) {
        return false;
    }

    if (peek_ec == net::error::eof || peek_ec == net::error::connection_reset ||
        peeked == 0) {
        open_ = false;
        return true;
    }

    if (peek_ec) {
        open_ = false;
        return true;
    }

    return false;
}

void ClientConnection::probe_disconnect() {
    (void)detect_peer_disconnect();
}

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

#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::forced_read_message) {
        std::string message = std::move(*test::SocketTestHooks::forced_read_message);
        test::SocketTestHooks::forced_read_message.reset();
        return message;
    }
#endif

    beast::error_code avail_ec;
    std::size_t available_bytes = 0;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::next_avail_error) {
        avail_ec = *test::SocketTestHooks::next_avail_error;
        test::SocketTestHooks::next_avail_error.reset();
    } else
#endif
    {
        available_bytes = beast::get_lowest_layer(ws_).available(avail_ec);
    }

    if (avail_ec) {
        std::cerr << "[SERVER-CONN-DIAG] try_read() available error: "
                  << avail_ec.message() << " (" << avail_ec.value() << ")\n";
        open_ = false;
        return std::nullopt;
    }

    if (available_bytes == 0) {
        if (detect_peer_disconnect()) {
            return std::nullopt;
        }
        return std::nullopt;
    }

    beast::flat_buffer buffer;
    beast::error_code read_ec;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::next_read_error) {
        read_ec = *test::SocketTestHooks::next_read_error;
        test::SocketTestHooks::next_read_error.reset();
    } else
#endif
    {
        ws_.read(buffer, read_ec);
    }

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
    if (detect_peer_disconnect()) {
        return false;
    }

    beast::error_code write_ec;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::next_write_error) {
        write_ec = *test::SocketTestHooks::next_write_error;
        test::SocketTestHooks::next_write_error.reset();
    } else
#endif
    {
        ws_.write(net::buffer(message), write_ec);
    }

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec == websocket::error::closed || write_ec == net::error::eof ||
        write_ec == net::error::connection_reset) {
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

void ClientConnection::close() {
    if (!open_) {
        return;
    }

    beast::error_code close_ec;
    beast::get_lowest_layer(ws_).close(close_ec);
    open_ = false;
}

}  // namespace kfc
