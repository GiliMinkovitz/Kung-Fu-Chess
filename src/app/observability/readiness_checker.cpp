#include "app/observability/readiness_checker.h"

#include "app/i_runtime_store.h"
#include "database/i_database_connection.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace kfc::app::observability {

namespace {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

struct ParsedHttpEndpoint {
    std::string host;
    std::string port;
};

std::optional<ParsedHttpEndpoint> parse_http_endpoint(const std::string& endpoint) {
    constexpr std::string_view prefix = "http://";
    if (endpoint.compare(0, prefix.size(), prefix) != 0) {
        return std::nullopt;
    }

    const std::string_view remainder = std::string_view(endpoint).substr(prefix.size());
    const std::size_t slash = remainder.find('/');
    const std::string_view host_port =
        slash == std::string_view::npos ? remainder : remainder.substr(0, slash);

    const std::size_t colon = host_port.find(':');
    if (colon == std::string_view::npos) {
        return ParsedHttpEndpoint{std::string(host_port), "80"};
    }

    return ParsedHttpEndpoint{std::string(host_port.substr(0, colon)),
                              std::string(host_port.substr(colon + 1))};
}

}  // namespace

bool is_redis_ready(const RedisConfig& redis_config, const IRuntimeStore& runtime_store) {
    if (!redis_config.enabled) {
        return true;
    }
    return runtime_store.is_available();
}

bool is_database_ready(const IDatabaseConnection& database) {
    return database.is_connected();
}

bool is_http_endpoint_reachable(const std::string& endpoint,
                                const std::chrono::milliseconds timeout) {
    const std::optional<ParsedHttpEndpoint> parsed = parse_http_endpoint(endpoint);
    if (!parsed.has_value()) {
        return false;
    }

    try {
        net::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);

        net::steady_timer timer(io_context);
        timer.expires_after(timeout);
        timer.async_wait([&stream](const beast::error_code& ec) {
            if (!ec) {
                beast::error_code ignored;
                stream.socket().cancel(ignored);
            }
        });

        const auto results = resolver.resolve(parsed->host, parsed->port);
        stream.connect(results);

        http::request<http::empty_body> request{http::verb::get, "/health", 11};
        request.set(http::field::host, parsed->host);
        http::write(stream, request);

        beast::flat_buffer buffer;
        http::response<http::string_body> response;
        http::read(stream, buffer, response);

        timer.cancel();
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);
        return response.result() == http::status::ok;
    } catch (...) {
        return false;
    }
}

bool check_gateway_ready(const IDatabaseConnection& database, const RedisConfig& redis_config,
                         const IRuntimeStore& runtime_store,
                         const std::string& matchmaker_health_endpoint) {
    if (!is_database_ready(database)) {
        return false;
    }
    if (!is_redis_ready(redis_config, runtime_store)) {
        return false;
    }
    return is_http_endpoint_reachable(matchmaker_health_endpoint, std::chrono::milliseconds(500));
}

bool check_matchmaker_ready(const IDatabaseConnection& database, const RedisConfig& redis_config,
                            const IRuntimeStore& runtime_store) {
    if (!is_database_ready(database)) {
        return false;
    }
    if (!is_redis_ready(redis_config, runtime_store)) {
        return false;
    }
    return !runtime_store.list_game_servers().empty();
}

bool check_game_server_ready(const IDatabaseConnection& database, const RedisConfig& redis_config,
                             const IRuntimeStore& runtime_store, const bool allocation_api_active) {
    if (!is_database_ready(database)) {
        return false;
    }
    if (!is_redis_ready(redis_config, runtime_store)) {
        return false;
    }
    return allocation_api_active;
}

}  // namespace kfc::app::observability
