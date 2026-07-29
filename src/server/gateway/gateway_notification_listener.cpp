#include "server/gateway/gateway_notification_listener.h"

#include "matchmaking/protocol/matchmaking_codec.h"
#include "server/gateway/gateway_notification_handler.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

namespace {

void close_socket(tcp::socket& socket) {
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
}

}  // namespace

GatewayNotificationListener::GatewayNotificationListener(std::string bind_address,
                                                         const unsigned short port,
                                                         GatewayNotificationHandler& handler)
    : bind_address_(std::move(bind_address)),
      configured_port_(port),
      notification_handler_(handler) {
    io_context_ = std::make_unique<net::io_context>();
    acceptor_ = std::make_unique<tcp::acceptor>(
        *io_context_, tcp::endpoint{net::ip::make_address(bind_address_), configured_port_});
    acceptor_->set_option(net::socket_base::reuse_address{true});
    acceptor_->non_blocking(true);
    bound_port_ = acceptor_->local_endpoint().port();
    thread_ = std::thread([this] { run_loop(); });
}

GatewayNotificationListener::~GatewayNotificationListener() {
    stop();
}

void GatewayNotificationListener::poll() {
    if (io_context_ != nullptr) {
        io_context_->poll();
    }
}

void GatewayNotificationListener::stop() {
    if (stop_requested_.exchange(true)) {
        return;
    }

    if (acceptor_ != nullptr) {
        beast::error_code ec;
        acceptor_->close(ec);
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    acceptor_.reset();
    io_context_.reset();
}

unsigned short GatewayNotificationListener::port() const {
    const unsigned short current = bound_port_.load();
    return current != 0 ? current : configured_port_;
}

void GatewayNotificationListener::handle_connection(tcp::socket socket) {
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::string_body> request;
    http::read(socket, buffer, request, ec);
    if (ec) {
        close_socket(socket);
        return;
    }

    http::response<http::string_body> response;
    response.version(request.version());
    response.keep_alive(false);
    response.set(http::field::content_type, "text/plain");

    if (request.method() != http::verb::post) {
        response.result(http::status::method_not_allowed);
        response.body() = "Method Not Allowed";
    } else if (request.target() == "/gateway/notify/match") {
        if (const std::optional<matchmaking::MatchNotification> notification =
                matchmaking::decode_match_notification(request.body())) {
            notification_handler_.notify_match_found(*notification);
            response.result(http::status::ok);
            response.body() = "OK";
        } else {
            response.result(http::status::bad_request);
            response.body() = "Bad Request";
        }
    } else if (request.target() == "/gateway/notify/timeout") {
        if (const std::optional<PlayerId> player_id =
                matchmaking::decode_timeout_notification(request.body())) {
            notification_handler_.notify_search_timeout(*player_id);
            response.result(http::status::ok);
            response.body() = "OK";
        } else {
            response.result(http::status::bad_request);
            response.body() = "Bad Request";
        }
    } else {
        response.result(http::status::not_found);
        response.body() = "Not Found";
    }

    response.prepare_payload();
    http::write(socket, response, ec);
    close_socket(socket);
}

void GatewayNotificationListener::run_loop() {
    while (!stop_requested_.load()) {
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
            if (stop_requested_.load()) {
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

}  // namespace kfc
