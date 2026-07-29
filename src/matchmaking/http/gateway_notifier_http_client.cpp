#include "matchmaking/http/gateway_notifier_http_client.h"

#include "app/observability/correlation_id.h"
#include "matchmaking/protocol/matchmaking_codec.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc::matchmaking {

namespace {

struct ParsedEndpoint {
    std::string host;
    std::string port;
    std::string target;
};

std::optional<ParsedEndpoint> parse_endpoint(const std::string& endpoint,
                                             const std::string& path) {
    constexpr std::string_view prefix = "http://";
    if (endpoint.compare(0, prefix.size(), prefix) != 0) {
        return std::nullopt;
    }

    const std::string_view remainder = std::string_view(endpoint).substr(prefix.size());
    const std::size_t slash = remainder.find('/');
    const std::string_view host_port =
        slash == std::string_view::npos ? remainder : remainder.substr(0, slash);
    const std::string target =
        slash == std::string_view::npos ? path : std::string(remainder.substr(slash));

    const std::size_t colon = host_port.find(':');
    if (colon == std::string_view::npos) {
        return ParsedEndpoint{std::string(host_port), "80", target};
    }

    return ParsedEndpoint{std::string(host_port.substr(0, colon)),
                          std::string(host_port.substr(colon + 1)), target};
}

bool post_text(const std::string& endpoint, const std::string& path, const std::string& body,
               const std::chrono::milliseconds timeout) {
    const std::optional<ParsedEndpoint> parsed = parse_endpoint(endpoint, path);
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

        http::request<http::string_body> request{http::verb::post, parsed->target, 11};
        request.set(http::field::host, parsed->host);
        request.set(http::field::content_type, "text/plain");
        if (!kfc::app::observability::current_correlation_id().empty()) {
            request.set(kfc::app::observability::kCorrelationIdHeader,
                        kfc::app::observability::current_correlation_id());
        }
        request.body() = body;
        request.prepare_payload();

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

}  // namespace

GatewayNotifierHttpClient::GatewayNotifierHttpClient(
    std::string gateway_notification_endpoint, const std::chrono::milliseconds timeout)
    : gateway_notification_endpoint_(std::move(gateway_notification_endpoint)),
      timeout_(timeout) {}

bool GatewayNotifierHttpClient::notify_match_found(const MatchNotification& notification) {
    return post_text(gateway_notification_endpoint_, "/gateway/notify/match",
                     encode_match_notification(notification), timeout_);
}

bool GatewayNotifierHttpClient::notify_search_timeout(const PlayerId player_id) {
    return post_text(gateway_notification_endpoint_, "/gateway/notify/timeout",
                     encode_timeout_notification(player_id), timeout_);
}

}  // namespace kfc::matchmaking
