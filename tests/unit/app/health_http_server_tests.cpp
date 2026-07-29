#include "app/health_http_server.h"
#include "app/server_metrics.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <doctest/doctest.h>

#include <string>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

struct HttpGetResponse {
    http::status status = http::status::unknown;
    std::string body;
};

HttpGetResponse http_get(const std::string& host, unsigned short port, const std::string& target) {
    net::io_context io_context;
    tcp::resolver resolver(io_context);
    const auto endpoints = resolver.resolve(host, std::to_string(port));

    tcp::socket socket(io_context);
    net::connect(socket, endpoints);

    http::request<http::empty_body> request{http::verb::get, target, 11};
    request.set(http::field::host, host);
    http::write(socket, request);

    beast::flat_buffer buffer;
    http::response<http::string_body> response;
    http::read(socket, buffer, response);

    return HttpGetResponse{response.result(), response.body()};
}

}  // namespace

TEST_CASE("HealthHttpServerTest - HealthEndpointReturnsOk") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    server.start();

    const HttpGetResponse response = http_get("127.0.0.1", server.port(), "/health");
    CHECK(response.status == http::status::ok);
    CHECK_EQ(response.body, "OK");

    server.stop();
}

TEST_CASE("HealthHttpServerTest - ReadyEndpointReturnsOk") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    server.start();

    const HttpGetResponse response = http_get("127.0.0.1", server.port(), "/ready");
    CHECK(response.status == http::status::ok);
    CHECK_EQ(response.body, "OK");

    server.stop();
}

TEST_CASE("HealthHttpServerTest - UnknownPathReturnsNotFound") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    server.start();

    const HttpGetResponse response = http_get("127.0.0.1", server.port(), "/unknown");
    CHECK(response.status == http::status::not_found);

    server.stop();
}

TEST_CASE("HealthHttpServerTest - MetricsEndpointReturnsPlainText") {
    kfc::app::HealthHttpServer server(
        "127.0.0.1", 0, []() {
            kfc::app::ServerMetrics metrics;
            metrics.active_rooms = 4;
            metrics.connected_sessions = 8;
            metrics.matchmaking_queue = 2;
            metrics.server_uptime_seconds = 153;
            metrics.last_tick_duration_ms = 16;
            metrics.server_id = "local";
            metrics.region = "local";
            return metrics;
        });
    server.start();

    const HttpGetResponse response = http_get("127.0.0.1", server.port(), "/metrics");
    CHECK(response.status == http::status::ok);
    CHECK_EQ(response.body,
             "active_rooms 4\n"
             "connected_sessions 8\n"
             "matchmaking_queue 2\n"
             "server_uptime_seconds 153\n"
             "last_tick_duration_ms 16\n"
             "server_id local\n"
             "region local\n"
             "endpoint \n"
             "redis_enabled 0\n"
             "redis_connected 0\n");

    server.stop();
}

TEST_CASE("HealthHttpServerTest - MetricsEndpointUnavailableWithoutProvider") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    server.start();

    const HttpGetResponse response = http_get("127.0.0.1", server.port(), "/metrics");
    CHECK(response.status == http::status::not_found);

    server.stop();
}

TEST_CASE("HealthHttpServerTest - StartsAndStopsCleanly") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    CHECK_FALSE(server.is_running());

    server.start();
    CHECK(server.is_running());
    CHECK_GT(server.port(), 0);

    server.stop();
    CHECK_FALSE(server.is_running());
}

TEST_CASE("HealthHttpServerTest - DestructorStopsRunningServer") {
    kfc::app::HealthHttpServer server("127.0.0.1", 0);
    server.start();
    CHECK(server.is_running());
}
