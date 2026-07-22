#include "server/game_server.h"

#include "logic/game_action.h"
#include "model/game_config.h"
#include "server/snapshot_writer.h"
#include "ui/view/board_view_builder.h"

#include <chrono>
#include <cctype>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace {

std::optional<std::string_view> next_token(std::string_view& message) {
    while (!message.empty() && std::isspace(static_cast<unsigned char>(message.front()))) {
        message.remove_prefix(1);
    }
    if (message.empty()) {
        return std::nullopt;
    }

    const std::size_t end = message.find_first_of(" \t\r\n", 0);
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

void process_session_messages(kfc::PlayerSession& session, kfc::Match& match) {
    if (const auto raw_message = session.connection()->try_read()) {
        if (const auto action = parse_message(*raw_message)) {
            match.submit_action(*action);
        }
    }
}

}  // namespace

namespace kfc {

GameServer::GameServer(unsigned short port, BoardModel default_board, const std::string& db_path)
    : websocket_server_(port),
      room_(std::move(default_board)),
      database_(db_path),
      player_repository_(database_) {
    if (!database_.open() || !database_.initialize_schema()) {
        throw std::runtime_error("Failed to initialize database");
    }
}

WebSocketServer& GameServer::websocket_server() noexcept {
    return websocket_server_;
}

Matchmaking& GameServer::matchmaking() noexcept {
    return matchmaking_;
}

GameRoom& GameServer::room() noexcept {
    return room_;
}

SqliteDatabase& GameServer::database() noexcept {
    return database_;
}

PlayerRepository& GameServer::player_repository() noexcept {
    return player_repository_;
}

void GameServer::accept_new_clients() {
    if (websocket_server_.clients().size() >= WebSocketServer::kMaxClients) {
        return;
    }

    const std::size_t before = websocket_server_.clients().size();
    websocket_server_.try_accept();
    if (websocket_server_.clients().size() <= before) {
        return;
    }

    ClientConnection& connection = websocket_server_.clients().back();
    sessions_.emplace_back(next_session_id_++, &connection, player_repository_);
    PlayerSession& session = sessions_.back();
    session.request_play();
    if (session.state() == PlayerSessionState::Searching) {
        matchmaking_.enqueue(session);
    }

    if (const auto matched = matchmaking_.try_create_match()) {
        (*matched)[0]->set_playing();
        (*matched)[1]->set_playing();
        room_.activate((*matched)[0], (*matched)[1]);
    }
}

void GameServer::prune_sessions() {
    websocket_server_.prune_disconnected();

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (!it->connection()->is_open()) {
            matchmaking_.remove(*it);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameServer::process_active_room(std::int64_t elapsed,
                                       std::chrono::steady_clock::time_point& last_tick) {
    Match& match = room_.match();

    if (elapsed >= kTargetFrameMs && !match.is_game_over()) {
        match.tick(elapsed);

        const BoardViewModel view = BoardViewBuilder::build(match.state());
        const std::string snapshot = write_snapshot(view);
        room_.white_session()->connection()->try_send(snapshot);
        room_.black_session()->connection()->try_send(snapshot);

        last_tick = std::chrono::steady_clock::now();
    }

    if (!match.is_game_over()) {
        process_session_messages(*room_.white_session(), match);
        process_session_messages(*room_.black_session(), match);
    }
}

void GameServer::finish_active_room() {
    PlayerSession* white = room_.white_session();
    PlayerSession* black = room_.black_session();

    room_.reset();

    if (white != nullptr) {
        white->connection()->close();
    }
    if (black != nullptr) {
        black->connection()->close();
    }
}

void GameServer::run() {
    std::cout << "Server started\n";

    auto last_tick = std::chrono::steady_clock::now();

    while (true) {
        accept_new_clients();

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick).count();

        if (room_.active()) {
            process_active_room(elapsed, last_tick);
            if (room_.match().is_game_over()) {
                finish_active_room();
            }
        }

        prune_sessions();

        if (elapsed < kTargetFrameMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

}  // namespace kfc
