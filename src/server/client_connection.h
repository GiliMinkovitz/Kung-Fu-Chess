#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <cstddef>
#include <optional>
#include <string>

namespace kfc {

class ClientConnection {
public:
    explicit ClientConnection(boost::asio::ip::tcp::socket socket);

    [[nodiscard]] bool is_open() const noexcept { return open_; }

    std::optional<std::string> try_read();
    bool try_send(const std::string& message);
    bool send_message(const std::string& message);
    void close();
    void probe_disconnect();

private:
    bool detect_peer_disconnect();
    void update_buffered_bytes(std::size_t available_before_read, std::size_t consumed_bytes);

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    // Bytes Beast has read from the socket but not yet delivered as messages.
    // Accounting assumes one frame per message (auto_fragment(false)), no
    // permessage-deflate; revisit if transport configuration changes.
    std::size_t buffered_bytes_ = 0;
    bool open_ = true;
    bool initial_snapshot_sent_ = false;
};

}  // namespace kfc
