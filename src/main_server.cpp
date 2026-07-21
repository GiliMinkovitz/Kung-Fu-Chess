#include "logic/game_action.h"
#include "model/board_model.h"
#include "model/game_config.h"
#include "server/match.h"
#include "server/snapshot_writer.h"
#include "server/websocket_server.h"
#include "ui/view/board_view_builder.h"

#include <chrono>
#include <cctype>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>

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

std::optional<std::string_view> next_token(std::string_view& message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    if (message.empty()) {
        return std::nullopt;
    }

    const std::size_t end =
        message.find_first_of(" \t\r\n", 0);
    const std::string_view token =
        end == std::string_view::npos ? message : message.substr(0, end);
    message.remove_prefix(token.size());
    return token;
}

std::optional<std::size_t> parse_non_negative_int(std::string_view token) {
    if (token.empty()) {
        return std::nullopt;
    }

    std::size_t value = 0;
    for (const char ch : token) {
        if (!std::isdigit(static_cast<unsigned char>(ch))) {
            return std::nullopt;
        }
        value = value * 10 + static_cast<std::size_t>(ch - '0');
    }
    return value;
}

std::optional<kfc::GameAction> parse_message(std::string_view message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.back()))) {
        message.remove_suffix(1);
    }

    if (message.empty()) {
        std::cerr << "Error: empty message\n";
        return std::nullopt;
    }

    const auto command = next_token(message);
    if (!command) {
        std::cerr << "Error: empty message\n";
        return std::nullopt;
    }

    if (*command == "clear") {
        if (next_token(message)) {
            std::cerr << "Error: 'clear' takes no arguments\n";
            return std::nullopt;
        }
        return kfc::ClearSelection{};
    }

    if (*command == "select" || *command == "move" || *command == "jump") {
        const auto row = next_token(message);
        const auto col = next_token(message);
        if (!row || !col) {
            std::cerr << "Error: '" << *command << "' requires row and col\n";
            return std::nullopt;
        }
        if (next_token(message)) {
            std::cerr << "Error: too many arguments for '" << *command << "'\n";
            return std::nullopt;
        }

        const auto parsed_row = parse_non_negative_int(*row);
        const auto parsed_col = parse_non_negative_int(*col);
        if (!parsed_row || !parsed_col) {
            std::cerr << "Error: row and col must be non-negative integers\n";
            return std::nullopt;
        }

        if (*command == "select") {
            return kfc::Select{*parsed_row, *parsed_col};
        }
        if (*command == "move") {
            return kfc::MoveSelected{*parsed_row, *parsed_col};
        }
        return kfc::JumpAt{*parsed_row, *parsed_col};
    }

    std::cerr << "Error: unknown command '" << *command << "'\n";
    return std::nullopt;
}

}  // namespace

int main() {
    try {
        kfc::Match match(default_board());
        kfc::WebSocketServer server{kServerPort};

        std::cout << "Server started\n";

        auto last_tick = std::chrono::steady_clock::now();

        while (!match.is_game_over()) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick)
                    .count();

            if (elapsed >= kfc::kTargetFrameMs) {
                match.tick(elapsed);

                if (!server.clients().empty()) {
                    const kfc::BoardViewModel view =
                        kfc::BoardViewBuilder::build(match.state());
                    const std::string snapshot = kfc::write_snapshot(view);
                    server.broadcast(snapshot);
                }

                last_tick = now;
            }

            if (server.clients().size() < kfc::WebSocketServer::kMaxClients) {
                server.try_accept();
            }

            for (kfc::ClientConnection& client : server.clients()) {
                if (const auto raw_message = client.try_read()) {
                    if (const auto action = parse_message(*raw_message)) {
                        match.submit_action(*action);
                    }
                }
            }

            server.prune_disconnected();

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
