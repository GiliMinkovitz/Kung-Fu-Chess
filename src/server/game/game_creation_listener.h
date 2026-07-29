#pragma once

#include "server/game/game_allocation_handler.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace kfc {

class GameCreationListener {
public:
    GameCreationListener(std::string bind_address, unsigned short port,
                         GameAllocationHandler& allocation_handler,
                         std::string expected_service_token = {});
    ~GameCreationListener();

    GameCreationListener(const GameCreationListener&) = delete;
    GameCreationListener& operator=(const GameCreationListener&) = delete;

    void poll();
    void stop();

    [[nodiscard]] unsigned short port() const;
    [[nodiscard]] bool is_active() const;

private:
    void run_loop();
    void handle_connection(boost::asio::ip::tcp::socket socket);

    std::string bind_address_;
    unsigned short configured_port_;
    GameAllocationHandler& allocation_handler_;
    std::string expected_service_token_;
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::thread thread_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<unsigned short> bound_port_{0};
};

}  // namespace kfc
