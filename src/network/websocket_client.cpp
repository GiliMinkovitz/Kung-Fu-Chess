#include "network/websocket_client.h"

#include "network/websocket_endpoint.h"

#ifdef KFC_TEST_BUILD
#include "test/socket_test_hooks.h"
#endif

#include <cstdint>
#include <stdexcept>
#include <chrono>
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

bool is_transient_io_error(const beast::error_code& ec) {
    return ec == net::error::would_block || ec == net::error::operation_aborted ||
           ec.value() == ECANCELED;
}

void websocket_write(websocket::stream<tcp::socket>& ws, net::const_buffer buffer,
                     beast::error_code& ec) {
    try {
        ws.write(buffer, ec);
    } catch (const beast::system_error& ex) {
        ec = ex.code();
    }
}

void websocket_read(websocket::stream<tcp::socket>& ws, beast::flat_buffer& buffer,
                    beast::error_code& ec) {
    try {
        ws.read(buffer, ec);
    } catch (const beast::system_error& ex) {
        ec = ex.code();
    }
}

// Wire size of one unmasked server-to-client frame for a payload of this size.
//
// buffered_bytes_ accounting assumptions:
// - One WebSocket message is transmitted as exactly one frame.
// - Relies on auto_fragment(false) on both peers.
// - Assumes permessage-deflate is disabled.
// - consumed_bytes is derived from payload length plus fixed header math.
// If transport configuration changes (fragmentation, compression, or a different
// read strategy), this accounting must be revisited.
std::size_t unmasked_frame_size(std::size_t payload_size) {
    std::size_t header_size = 2;
    if (payload_size > 65535) {
        header_size += 8;
    } else if (payload_size > 125) {
        header_size += 2;
    }
    return header_size + payload_size;
}

}  // namespace

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
        websocket::stream_base::timeout timeout{};
        timeout.handshake_timeout = std::chrono::seconds(30);
        timeout.keep_alive_pings = false;
        ws_->set_option(timeout);

        beast::error_code handshake_ec;
        ws_->handshake(host_, "/", handshake_ec);
        if (handshake_ec) {
            ws_.reset();
            throw beast::system_error{handshake_ec};
        }

        // One frame per message keeps the buffered-byte accounting exact.
        ws_->auto_fragment(false);
        buffered_bytes_ = 0;
        connected_ = true;
#ifndef KFC_TEST_BUILD
        std::cerr << "[DIAG] WebSocketClient::connect() succeeded\n";
#endif
    } catch (const std::exception& ex) {
#ifndef KFC_TEST_BUILD
        std::cerr << "[DIAG] WebSocketClient::connect() failed: " << ex.what()
                  << '\n';
#endif
        throw;
    }
}

void WebSocketClient::disconnect() {
    if (!ws_) {
        connected_ = false;
        return;
    }

    beast::error_code close_ec;
    beast::get_lowest_layer(*ws_).close(close_ec);
    ws_.reset();
    buffered_bytes_ = 0;
    connected_ = false;
}

void WebSocketClient::connect_to_game_server(const std::string& endpoint) {
    const std::optional<WebSocketEndpoint> parsed = parse_websocket_endpoint(endpoint);
    if (!parsed.has_value()) {
        throw std::invalid_argument("Invalid game server endpoint: " + endpoint);
    }

    disconnect();
    host_ = parsed->host;
    port_ = parsed->port;
    connect();
}

bool WebSocketClient::try_send(const std::string& message) {
    if (!connected_ || !ws_) {
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
        websocket_write(*ws_, net::buffer(message), write_ec);
    }

    if (is_transient_io_error(write_ec)) {
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

    auto& socket = beast::get_lowest_layer(*ws_);

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
#ifndef KFC_TEST_BUILD
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() available error: "
                  << avail_ec.message() << " (" << avail_ec.value() << ")\n";
#endif
        buffered_bytes_ = 0;
        connected_ = false;
        return std::nullopt;
    }

    // Beast pulls whole segments out of the socket into its own read buffer, so
    // a complete message can be waiting there while the socket reports nothing
    // available. buffered_bytes_ tracks those leftovers; reading is safe when
    // either source has data.
    if (available_bytes == 0 && buffered_bytes_ == 0) {
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
                connected_ = false;
                return std::nullopt;
            }
        }

        beast::error_code peek_ec;
        char byte = 0;
        std::size_t peeked = 0;
#ifdef KFC_TEST_BUILD
        if (test::SocketTestHooks::next_peek_error) {
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
            return std::nullopt;
        }

        if (peek_ec == net::error::eof || peek_ec == net::error::connection_reset ||
            (peeked == 0 && !peek_ec)) {
#ifndef KFC_TEST_BUILD
            std::cerr << "[CLIENT-DIAG] try_receive_snapshot() websocket closed/error: "
                      << peek_ec.message() << " (" << peek_ec.value() << ")\n";
#endif
            connected_ = false;
            return std::nullopt;
        }

        if (peek_ec) {
#ifndef KFC_TEST_BUILD
            std::cerr << "[CLIENT-DIAG] try_receive_snapshot() peek error: "
                      << peek_ec.message() << " (" << peek_ec.value() << ")\n";
#endif
            connected_ = false;
            return std::nullopt;
        }
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
        websocket_read(*ws_, buffer, read_ec);
    }

    if (is_transient_io_error(read_ec)) {
        return std::nullopt;
    }

    if (read_ec == websocket::error::closed || read_ec == net::error::eof ||
        read_ec == net::error::connection_reset) {
#ifndef KFC_TEST_BUILD
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() websocket closed/error: "
                  << read_ec.message() << " (" << read_ec.value() << ")\n";
#endif
        buffered_bytes_ = 0;
        connected_ = false;
        return std::nullopt;
    }

    if (read_ec) {
#ifndef KFC_TEST_BUILD
        std::cerr << "[CLIENT-DIAG] try_receive_snapshot() websocket read error: "
                  << read_ec.message() << " (" << read_ec.value() << ")\n";
#endif
        buffered_bytes_ = 0;
        connected_ = false;
        throw beast::system_error{read_ec};
    }

    std::string message = beast::buffers_to_string(buffer.data());
    update_buffered_bytes(available_bytes, unmasked_frame_size(message.size()));
    return message;
}

// Updates buffered_bytes_ after a successful read. See unmasked_frame_size() for
// the assumptions this accounting relies on; revisit both if transport config changes.
void WebSocketClient::update_buffered_bytes(std::size_t available_before_read,
                                            std::size_t consumed_bytes) {
    beast::error_code remaining_ec;
    const std::size_t available_after_read = beast::get_lowest_layer(*ws_).available(remaining_ec);

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

}  // namespace kfc
