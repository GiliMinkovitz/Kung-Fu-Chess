#include "server/game/game_allocation_http_client.h"

#include "server/game/protocol/game_creation_codec.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include <optional>
#include <string>
#include <utility>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace kfc {

namespace {

constexpr const char* kServiceTokenHeader = "X-KFC-Service-Token";

struct ParsedAllocationEndpoint {
    std::string host;
    std::string port;
    std::string target;
};

std::optional<ParsedAllocationEndpoint> parse_allocation_endpoint(
    const std::string& allocation_endpoint) {
    constexpr std::string_view prefix = "http://";
    if (allocation_endpoint.compare(0, prefix.size(), prefix) != 0) {
        return std::nullopt;
    }

    const std::string_view remainder = std::string_view(allocation_endpoint).substr(prefix.size());
    const std::size_t slash = remainder.find('/');
    const std::string_view host_port =
        slash == std::string_view::npos ? remainder : remainder.substr(0, slash);
    const std::string target =
        slash == std::string_view::npos ? "/allocate" : std::string(remainder.substr(slash));

    const std::size_t colon = host_port.find(':');
    if (colon == std::string_view::npos) {
        return ParsedAllocationEndpoint{std::string(host_port), "80", target};
    }

    return ParsedAllocationEndpoint{std::string(host_port.substr(0, colon)),
                                    std::string(host_port.substr(colon + 1)), target};
}

}  // namespace

GameAllocationHttpClient::GameAllocationHttpClient(std::string service_token,
                                                   const std::chrono::milliseconds timeout)
    : service_token_(std::move(service_token)), timeout_(timeout) {}

std::optional<GameCreationResponse> GameAllocationHttpClient::allocate(
    const std::string& allocation_endpoint, const GameCreationRequest& request) const {
    const std::optional<ParsedAllocationEndpoint> parsed =
        parse_allocation_endpoint(allocation_endpoint);
    if (!parsed.has_value()) {
        return std::nullopt;
    }

    try {
        net::io_context io_context;
        tcp::resolver resolver(io_context);
        beast::tcp_stream stream(io_context);

        net::steady_timer timer(io_context);
        timer.expires_after(timeout_);
        timer.async_wait([&stream](const beast::error_code& ec) {
            if (!ec) {
                beast::error_code ignored;
                stream.socket().cancel(ignored);
            }
        });

        const auto results = resolver.resolve(parsed->host, parsed->port);
        stream.connect(results);

        http::request<http::string_body> http_request{http::verb::post, parsed->target, 11};
        http_request.set(http::field::host, parsed->host);
        http_request.set(http::field::content_type, "text/plain");
        if (!service_token_.empty()) {
            http_request.set(kServiceTokenHeader, service_token_);
        }
        http_request.body() = encode_game_creation_request(request);
        http_request.prepare_payload();

        http::write(stream, http_request);

        beast::flat_buffer buffer;
        http::response<http::string_body> http_response;
        http::read(stream, buffer, http_response);

        timer.cancel();
        beast::error_code ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, ec);

        if (http_response.result() != http::status::ok) {
            return std::nullopt;
        }

        return decode_game_creation_response(http_response.body());
    } catch (...) {
        return std::nullopt;
    }
}

}  // namespace kfc
