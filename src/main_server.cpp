#include "model/board_model.h"
#include "model/game_config.h"
#include "server/match.h"

#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace {

constexpr unsigned short kServerPort = 8765;

kfc::BoardModel default_board() {
    return kfc::BoardModel::from_token_grid({
        {"bR", "bN", "bB", "bQ", "bK", "bB", "bN", "bR"},
        {"bP", "bP", "bP", "bP", "bP", "bP", "bP", "bP"},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {".", ".", ".", ".", ".", ".", ".", "."},
        {"wP", "wP", "wP", "wP", "wP", "wP", "wP", "wP"},
        {"wR", "wN", "wB", "wQ", "wK", "wB", "wN", "wR"},
    });
}

}  // namespace

int main() {
    try {
        kfc::Match match(default_board());

        net::io_context io_context;
        tcp::acceptor acceptor{
            io_context,
            tcp::endpoint{net::ip::make_address("127.0.0.1"), kServerPort},
        };
        acceptor.non_blocking(true);

        std::cout << "Server started\n";

        std::unique_ptr<websocket::stream<tcp::socket>> ws;
        bool client_connected_logged = false;
        bool client_disconnected_logged = false;

        auto last_tick = std::chrono::steady_clock::now();

        while (!match.is_game_over()) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick)
                    .count();

            if (elapsed >= kfc::kTargetFrameMs) {
                match.tick(elapsed);

                std::cout << "Clock: " << match.state().clock_ms() << '\n';
                std::cout << "GameOver: " << (match.is_game_over() ? "true" : "false")
                          << '\n';

                last_tick = now;
            }

            if (!ws) {
                beast::error_code accept_ec;
                tcp::socket socket{acceptor.get_executor()};
                acceptor.accept(socket, accept_ec);

                if (!accept_ec) {
                    ws = std::make_unique<websocket::stream<tcp::socket>>(std::move(socket));

                    beast::error_code handshake_ec;
                    ws->accept(handshake_ec);
                    if (handshake_ec) {
                        ws.reset();
                        throw beast::system_error{handshake_ec};
                    }

                    beast::get_lowest_layer(*ws).non_blocking(true);

                    if (!client_connected_logged) {
                        std::cout << "Client connected\n";
                        client_connected_logged = true;
                    }
                } else if (accept_ec != net::error::would_block) {
                    throw beast::system_error{accept_ec};
                }
            } else {
                beast::flat_buffer buffer;
                beast::error_code read_ec;
                ws->read(buffer, read_ec);

                if (read_ec == net::error::would_block) {
                    // No message yet; keep simulating.
                } else if (read_ec == websocket::error::closed) {
                    if (!client_disconnected_logged) {
                        std::cout << "Client disconnected\n";
                        client_disconnected_logged = true;
                    }
                    ws.reset();
                } else if (read_ec) {
                    throw beast::system_error{read_ec};
                } else {
                    std::cout << beast::buffers_to_string(buffer.data()) << '\n';
                }
            }

            if (elapsed < kfc::kTargetFrameMs) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
