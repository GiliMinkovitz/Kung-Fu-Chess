#include "matchmaking/http/matchmaking_http_server.h"

#include "app/observability/observability.h"
#include "app/observability/correlation_id.h"
#include "matchmaking/protocol/matchmaking_codec.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>
#include <iostream>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc::matchmaking {

namespace {

void close_socket(tcp::socket& socket) {
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);
    socket.close(ec);
}

}  // namespace

MatchmakingHttpServer::MatchmakingHttpServer(std::string bind_address, const unsigned short port,
                                             IMatchmakingService& matchmaking_service)
    : bind_address_(std::move(bind_address)),
      configured_port_(port),
      matchmaking_service_(matchmaking_service) {
    io_context_ = std::make_unique<net::io_context>();
    acceptor_ = std::make_unique<tcp::acceptor>(
        *io_context_, tcp::endpoint{net::ip::make_address(bind_address_), configured_port_});
    acceptor_->set_option(net::socket_base::reuse_address{true});
    acceptor_->non_blocking(true);
    bound_port_ = acceptor_->local_endpoint().port();
    thread_ = std::thread([this] { run_loop(); });
}

MatchmakingHttpServer::~MatchmakingHttpServer() {
    stop();
}

void MatchmakingHttpServer::poll() {
    if (io_context_ != nullptr) {
        io_context_->poll();
    }
}

void MatchmakingHttpServer::stop() {
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

unsigned short MatchmakingHttpServer::port() const {
    const unsigned short current = bound_port_.load();
    return current != 0 ? current : configured_port_;
}

void MatchmakingHttpServer::handle_connection(tcp::socket socket) {
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

    if (request.method() != http::verb::post) {
        response.result(http::status::method_not_allowed);
        response.body() = "Method Not Allowed";
    } else if (request.target() == "/matchmaking/join") {
        if (const std::optional<MatchRequest> decoded = decode_match_request(request.body())) {
            response.result(http::status::ok);
            response.body() = encode_match_response(matchmaking_service_.enter_queue(*decoded));
        } else {
            response.result(http::status::bad_request);
            response.body() = "Bad Request";
        }
    } else if (request.target() == "/matchmaking/leave") {
        if (const std::optional<MatchRequest> decoded = decode_match_request(request.body())) {
            matchmaking_service_.leave_queue(decoded->player_id, decoded->region);
            response.result(http::status::ok);
            response.body() = encode_match_response(MatchResponse{MatchJoinStatus::Queued, "left"});
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

void MatchmakingHttpServer::run_loop() {
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

}  // namespace kfc::matchmaking
