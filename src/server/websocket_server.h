#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <memory>
#include <optional>
#include <string>

namespace kfc {

class WebSocketServer {
public:
    explicit WebSocketServer(unsigned short port);

    [[nodiscard]] bool is_connected() const noexcept { return ws_ != nullptr; }

    void try_accept();
    std::optional<std::string> try_read();
    bool try_send(const std::string& message);

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::unique_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> ws_;
    bool client_connected_logged_ = false;
    bool client_disconnected_logged_ = false;
};

}  // namespace kfc
