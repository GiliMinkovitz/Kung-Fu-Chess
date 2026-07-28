#pragma once

#include "app/server_metrics.h"

#include <boost/asio/ip/tcp.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace boost::asio {
class io_context;

namespace ip::tcp {
class acceptor;
}  // namespace ip::tcp
}  // namespace boost::asio

namespace kfc::app {

using MetricsProvider = std::function<ServerMetrics()>;

class HealthHttpServer {
public:
    HealthHttpServer(std::string bind_address, unsigned short port,
                     MetricsProvider metrics_provider = {});
    ~HealthHttpServer();

    HealthHttpServer(const HealthHttpServer&) = delete;
    HealthHttpServer& operator=(const HealthHttpServer&) = delete;

    void start();
    void stop();

    [[nodiscard]] bool is_running() const noexcept { return running_; }
    [[nodiscard]] unsigned short port() const;

private:
    void run_loop();
    void handle_connection(boost::asio::ip::tcp::socket socket);

    std::string bind_address_;
    unsigned short port_;
    MetricsProvider metrics_provider_;
    std::atomic<bool> stop_requested_{false};
    bool running_ = false;
    std::thread thread_;
    std::unique_ptr<boost::asio::io_context> io_context_;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
};

}  // namespace kfc::app
