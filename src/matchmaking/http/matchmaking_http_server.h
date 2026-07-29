#pragma once

#include "matchmaking/i_matchmaking_service.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace kfc::matchmaking {

class MatchmakingHttpServer {
public:
    MatchmakingHttpServer(std::string bind_address, unsigned short port,
                            IMatchmakingService& matchmaking_service);
    ~MatchmakingHttpServer();

    MatchmakingHttpServer(const MatchmakingHttpServer&) = delete;
    MatchmakingHttpServer& operator=(const MatchmakingHttpServer&) = delete;

    void poll();
    void stop();

    [[nodiscard]] unsigned short port() const;

private:
    void run_loop();
    void handle_connection(boost::asio::ip::tcp::socket socket);

    std::string bind_address_;
    unsigned short configured_port_;
    IMatchmakingService& matchmaking_service_;
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
    std::thread thread_;
    std::atomic<bool> stop_requested_{false};
    std::atomic<unsigned short> bound_port_{0};
};

}  // namespace kfc::matchmaking
