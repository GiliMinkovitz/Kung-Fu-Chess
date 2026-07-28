#include "server/client_connection.h"

#ifdef KFC_TEST_BUILD
#include "test/socket_test_hooks.h"
#endif

#include "app/runtime_diagnostics.h"

#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;

namespace kfc {

namespace {

void websocket_write(websocket::stream<net::ip::tcp::socket>& ws, net::const_buffer buffer,
                     beast::error_code& ec) {
    try {
        ws.write(buffer, ec);
    } catch (const beast::system_error& ex) {
        ec = ex.code();
    }
}

void websocket_read(websocket::stream<net::ip::tcp::socket>& ws, beast::flat_buffer& buffer,
                    beast::error_code& ec) {
    try {
        ws.read(buffer, ec);
    } catch (const beast::system_error& ex) {
        ec = ex.code();
    }
}

// Wire size of one masked client-to-server frame for a payload of this size.
//
// buffered_bytes_ accounting assumptions:
// - One WebSocket message is transmitted as exactly one frame.
// - Relies on auto_fragment(false) on both peers.
// - Assumes permessage-deflate is disabled.
// - consumed_bytes is derived from payload length plus fixed header math.
// If transport configuration changes (fragmentation, compression, or a different
// read strategy), this accounting must be revisited.
std::size_t masked_frame_size(std::size_t payload_size) {
    std::size_t header_size = 2 + 4;
    if (payload_size > 65535) {
        header_size += 8;
    } else if (payload_size > 125) {
        header_size += 2;
    }
    return header_size + payload_size;
}

}  // namespace

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

    if (available_bytes > 0 || buffered_bytes_ > 0) {
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
        (peeked == 0 && !peek_ec)) {
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

    // One frame per message keeps the buffered-byte accounting exact.
    ws_.auto_fragment(false);
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
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cerr << "[SERVER-CONN-DIAG] try_read() available error: "
                      << avail_ec.message() << " (" << avail_ec.value() << ")\n";
        }
#endif
        buffered_bytes_ = 0;
        open_ = false;
        return std::nullopt;
    }

    // Beast pulls whole segments out of the socket into its own read buffer, so
    // a complete message can be waiting there while the socket reports nothing
    // available. buffered_bytes_ tracks those leftovers; reading is safe when
    // either source has data.
    if (available_bytes == 0 && buffered_bytes_ == 0) {
        (void)detect_peer_disconnect();
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
        websocket_read(ws_, buffer, read_ec);
    }

    if (read_ec == net::error::would_block) {
        return std::nullopt;
    }

    if (read_ec == websocket::error::closed) {
        buffered_bytes_ = 0;
        open_ = false;
        return std::nullopt;
    }

    if (read_ec) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cerr << "[SERVER-CONN-DIAG] try_read() websocket error: "
                      << read_ec.message() << " (" << read_ec.value() << ")\n";
        }
#endif
        buffered_bytes_ = 0;
        open_ = false;
        return std::nullopt;
    }

    std::string message = beast::buffers_to_string(buffer.data());
    update_buffered_bytes(available_bytes, masked_frame_size(message.size()));
    return message;
}

// Updates buffered_bytes_ after a successful read. See masked_frame_size() for
// the assumptions this accounting relies on; revisit both if transport config changes.
void ClientConnection::update_buffered_bytes(std::size_t available_before_read,
                                             std::size_t consumed_bytes) {
    beast::error_code remaining_ec;
    const std::size_t available_after_read = beast::get_lowest_layer(ws_).available(remaining_ec);

    // Bytes that moved from the socket into Beast's read buffer. Data arriving
    // mid-read only makes this smaller, never larger, so the running total can
    // never overestimate what is still pending.
    std::size_t moved_into_beast = 0;
    if (!remaining_ec && available_before_read > available_after_read) {
        moved_into_beast = available_before_read - available_after_read;
    }

    const std::size_t reachable = buffered_bytes_ + moved_into_beast;
    buffered_bytes_ = reachable > consumed_bytes ? reachable - consumed_bytes : 0;
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
        websocket_write(ws_, net::buffer(message), write_ec);
    }

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec == websocket::error::closed || write_ec == net::error::eof ||
        write_ec == net::error::connection_reset) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cerr << "[SERVER-CONN-DIAG] try_send() websocket error: "
                      << write_ec.message() << " (" << write_ec.value() << ")\n";
        }
#endif
        open_ = false;
        return false;
    }

    if (write_ec) {
#ifndef KFC_TEST_BUILD
        if (app::diagnostics_enabled()) {
            std::cerr << "[SERVER-CONN-DIAG] try_send() websocket error: "
                      << write_ec.message() << " (" << write_ec.value() << ")\n";
        }
#endif
        open_ = false;
        return false;
    }

    return true;
}

bool ClientConnection::send_message(const std::string& message) {
    beast::error_code write_ec;
#ifdef KFC_TEST_BUILD
    if (test::SocketTestHooks::next_write_error) {
        write_ec = *test::SocketTestHooks::next_write_error;
        test::SocketTestHooks::next_write_error.reset();
    } else
#endif
    {
        websocket_write(ws_, net::buffer(message), write_ec);
    }

    if (write_ec == net::error::would_block) {
        return false;
    }

    if (write_ec == websocket::error::closed || write_ec == net::error::eof ||
        write_ec == net::error::connection_reset) {
        open_ = false;
        return false;
    }

    if (write_ec) {
        open_ = false;
        return false;
    }

    open_ = true;
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
