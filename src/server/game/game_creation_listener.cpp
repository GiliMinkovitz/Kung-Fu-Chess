#include "server/game/game_creation_listener.h"

#include "app/observability/correlation_id.h"
#include "app/observability/metric_counters.h"
#include "app/observability/observability.h"
#include "app/observability/structured_logger.h"
#include "server/game/protocol/game_creation_codec.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

namespace {

constexpr const char* kServiceTokenHeader = "X-KFC-Service-Token";

void close_socket(tcp::socket& socket) {
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
}

bool service_token_is_valid(const http::request<http::string_body>& request,
                            const std::string& expected_service_token) {
    if (expected_service_token.empty()) {
        return true;
    }

    const auto token_it = request.find(kServiceTokenHeader);
    if (token_it == request.end()) {
        return false;
    }

    return token_it->value() == expected_service_token;
}

}  // namespace

GameCreationListener::GameCreationListener(std::string bind_address, const unsigned short port,
                                           GameAllocationHandler& allocation_handler,
                                           std::string expected_service_token)
    : bind_address_(std::move(bind_address)),
      configured_port_(port),
      allocation_handler_(allocation_handler),
      expected_service_token_(std::move(expected_service_token)) {
    io_context_ = std::make_unique<net::io_context>();
    acceptor_ = std::make_unique<tcp::acceptor>(
        *io_context_, tcp::endpoint{net::ip::make_address(bind_address_), configured_port_});
    acceptor_->set_option(net::socket_base::reuse_address{true});
    acceptor_->non_blocking(true);
    bound_port_ = acceptor_->local_endpoint().port();

    thread_ = std::thread([this] { run_loop(); });
}

GameCreationListener::~GameCreationListener() {
    stop();
}

void GameCreationListener::poll() {
    if (io_context_ != nullptr) {
        io_context_->poll();
    }
}

void GameCreationListener::stop() {
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

unsigned short GameCreationListener::port() const {
    const unsigned short current = bound_port_.load();
    return current != 0 ? current : configured_port_;
}

bool GameCreationListener::is_active() const {
    return !stop_requested_.load() && acceptor_ != nullptr && acceptor_->is_open();
}

void GameCreationListener::handle_connection(tcp::socket socket) {
    beast::error_code ec;
    beast::flat_buffer buffer;
    http::request<http::string_body> request;
    http::read(socket, buffer, request, ec);
    if (ec) {
        close_socket(socket);
        return;
    }

    kfc::app::observability::extract_correlation_id_from_request(request);
    const kfc::app::observability::CorrelationScope correlation_scope{
        kfc::app::observability::current_correlation_id()};

    http::response<http::string_body> response;
    response.version(request.version());
    response.keep_alive(false);
    response.set(http::field::content_type, "text/plain");

    if (request.method() != http::verb::post || request.target() != "/allocate") {
        response.result(http::status::not_found);
        response.body() = "Not Found";
    } else if (!service_token_is_valid(request, expected_service_token_)) {
        response.result(http::status::unauthorized);
        response.body() = "Unauthorized";
    } else if (const std::optional<GameCreationRequest> decoded =
                   decode_game_creation_request(request.body())) {
        kfc::app::observability::metrics().allocation_requests_total.fetch_add(
            1, std::memory_order_relaxed);
        kfc::app::observability::logger().log(kfc::app::observability::LogLevel::Info,
                                              "allocation_started");
        const GameCreationResponse created = allocation_handler_.allocate(*decoded);
        response.result(http::status::ok);
        response.body() = encode_game_creation_response(created);
    } else {
        response.result(http::status::bad_request);
        response.body() = "Bad Request";
    }

    response.prepare_payload();
    http::write(socket, response, ec);
    close_socket(socket);
}

void GameCreationListener::run_loop() {
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
