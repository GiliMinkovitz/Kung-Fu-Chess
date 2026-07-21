#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

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

    [[nodiscard]] bool is_connected() const noexcept { return connected_; }

    bool try_send(const std::string& message);
    std::optional<std::string> try_receive_snapshot();

private:
    std::string host_;
    std::uint16_t port_;
    boost::asio::io_context io_context_;
    std::optional<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws_;
    bool connected_ = false;
};

}  // namespace kfc
