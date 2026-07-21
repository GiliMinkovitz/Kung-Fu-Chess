#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <optional>
#include <string>

namespace kfc {

class ClientConnection {
public:
    explicit ClientConnection(boost::asio::ip::tcp::socket socket);

    [[nodiscard]] bool is_open() const noexcept { return open_; }

    std::optional<std::string> try_read();
    bool try_send(const std::string& message);

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    bool open_ = true;
};

}  // namespace kfc
