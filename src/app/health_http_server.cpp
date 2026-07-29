#include "app/health_http_server.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <future>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc::app {

namespace {

void close_socket(tcp::socket& socket) {
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
}

}  // namespace

HealthHttpServer::HealthHttpServer(std::string bind_address, unsigned short port,
                                   MetricsProvider metrics_provider,
                                   ReadinessProvider readiness_provider)
    : bind_address_(std::move(bind_address)),
      port_(port),
      metrics_provider_(std::move(metrics_provider)),
      readiness_provider_(std::move(readiness_provider)) {}

HealthHttpServer::~HealthHttpServer() {
    stop();
}

void HealthHttpServer::start() {
    if (running_) {
        return;
    }

    stop_requested_ = false;
    io_context_ = std::make_unique<net::io_context>();
    acceptor_ = std::make_unique<tcp::acceptor>(
        *io_context_, tcp::endpoint{net::ip::make_address(bind_address_), port_});
    acceptor_->set_option(net::socket_base::reuse_address{true});
    acceptor_->non_blocking(true);

    auto ready = std::make_shared<std::promise<void>>();
    std::future<void> ready_future = ready->get_future();
    thread_ = std::thread([this, ready] {
        ready->set_value();
        run_loop();
    });
    ready_future.wait();
    running_ = true;
}

void HealthHttpServer::stop() {
    if (!running_) {
        return;
    }

    stop_requested_ = true;

    if (acceptor_ != nullptr) {
        beast::error_code ec;
        acceptor_->close(ec);
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    acceptor_.reset();
    io_context_.reset();
    running_ = false;
}

unsigned short HealthHttpServer::port() const {
    if (acceptor_ == nullptr) {
        return port_;
    }
    return acceptor_->local_endpoint().port();
}

void HealthHttpServer::handle_connection(tcp::socket socket) {
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::empty_body> request;
    http::read(socket, buffer, request, ec);
    if (ec) {
        close_socket(socket);
        return;
    }

    http::response<http::string_body> response;
    response.version(request.version());
    response.keep_alive(false);

    if (request.method() == http::verb::get) {
        if (request.target() == "/health") {
            response.result(http::status::ok);
            response.set(http::field::content_type, "text/plain");
            response.body() = "OK";
        } else if (request.target() == "/ready") {
            const bool ready = !readiness_provider_ || readiness_provider_();
            response.result(ready ? http::status::ok : http::status::service_unavailable);
            response.set(http::field::content_type, "text/plain");
            response.body() = ready ? "OK" : "Not Ready";
        } else if (request.target() == "/metrics" && metrics_provider_) {
            response.result(http::status::ok);
            response.set(http::field::content_type, "text/plain");
            response.body() = format_server_metrics(metrics_provider_());
        } else {
            response.result(http::status::not_found);
            response.set(http::field::content_type, "text/plain");
            response.body() = "Not Found";
        }
    } else {
        response.result(http::status::method_not_allowed);
        response.set(http::field::content_type, "text/plain");
        response.body() = "Method Not Allowed";
    }

    response.prepare_payload();
    http::write(socket, response, ec);
    close_socket(socket);
}

void HealthHttpServer::run_loop() {
    while (!stop_requested_) {
        if (acceptor_ == nullptr || !acceptor_->is_open()) {
            break;
        }

        beast::error_code ec;
        tcp::socket socket(acceptor_->get_executor());
        acceptor_->accept(socket, ec);

        if (ec == net::error::would_block) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        if (ec) {
            if (stop_requested_) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        try {
            handle_connection(std::move(socket));
        } catch (...) {
        }
    }
}

}  // namespace kfc::app
