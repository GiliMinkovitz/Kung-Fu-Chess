#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace kfc {

// Transport-only WebSocket client. Sends and receives opaque text frames;
// does not parse snapshots or know about game types.
class WebSocketClient {
public:
    WebSocketClient(std::string host, std::uint16_t port);

    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    void connect();
    void disconnect();
    void connect_to_game_server(const std::string& endpoint);

    [[nodiscard]] bool is_connected() const noexcept { return connected_; }

    bool try_send(const std::string& message);
    std::optional<std::string> try_receive_snapshot();

private:
    void update_buffered_bytes(std::size_t available_before_read, std::size_t consumed_bytes);

    std::string host_;
    std::uint16_t port_;
    boost::asio::io_context io_context_;
    std::optional<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws_;
    // Bytes Beast has read from the socket but not yet delivered as messages.
    // Accounting assumes one frame per message (auto_fragment(false)), no
    // permessage-deflate; revisit if transport configuration changes.
    std::size_t buffered_bytes_ = 0;
    bool connected_ = false;
};

}  // namespace kfc
