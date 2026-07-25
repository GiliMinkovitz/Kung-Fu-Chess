#include "server/game_server.h"

#include "model/game_config.h"
#include "server/game_message_parser.h"
#include "server/snapshot_writer.h"
#include "ui/view/board_view_builder.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace {

void process_session_messages(kfc::PlayerSession& session, kfc::Match& match) {
    if (const auto raw_message = session.connection()->try_read()) {
        if (const auto action = kfc::parse_message(*raw_message)) {
            if (kfc::is_action_allowed(session, match, *action)) {
                match.submit_action(*action);
            }
        }
    }
}

}  // namespace

namespace kfc {

GameServer::GameServer(unsigned short port, BoardModel default_board, const std::string& db_path)
    : websocket_server_(port),
      room_(std::move(default_board)),
      database_(db_path),
      player_repository_(database_),
      game_repository_(database_) {
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

GameRepository& GameServer::game_repository() noexcept {
    return game_repository_;
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
    sessions_.emplace_back(next_session_id_++, &connection);
}

void GameServer::process_pending_logins() {
    for (PlayerSession& session : sessions_) {
        if (session.state() == PlayerSessionState::Playing ||
            !session.connection()->is_open()) {
            continue;
        }

        if (const auto raw_message = session.connection()->try_read()) {
            if (session.state() == PlayerSessionState::Searching) {
                parse_play_message(*raw_message);
                continue;
            }

            if (!session.has_player()) {
                if (const auto username = parse_login_message(*raw_message)) {
                    session.login(*username, player_repository_);
                }
                continue;
            }

            if (parse_play_message(*raw_message)) {
                session.request_play();
                if (session.state() == PlayerSessionState::Searching) {
                    const auto now = std::chrono::steady_clock::now();
                    if (const auto matched = matchmaking_.enqueue(session, now)) {
                        (*matched)[0]->connection()->try_send("match_found white");
                        (*matched)[1]->connection()->try_send("match_found black");
                        (*matched)[0]->set_playing();
                        (*matched)[1]->set_playing();
                        room_.activate((*matched)[0], (*matched)[1], game_repository_);
                        (*matched)[0]->connection()->try_send("game_start white");
                        (*matched)[1]->connection()->try_send("game_start black");
                    } else {
                        session.connection()->try_send("searching");
                    }
                }
            }
        }
    }
}

void GameServer::process_matchmaking_timeouts() {
    const auto now = std::chrono::steady_clock::now();
    for (PlayerSession* session : matchmaking_.check_timeouts(now)) {
        std::cout << "Matchmaking timeout for session " << session->id() << " (player "
                  << session->player().username() << ")\n";
        session->connection()->try_send("search_timeout");
        session->cancel_search();
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
    if (const std::optional<int> game_id = room_.db_game_id()) {
        if (const std::optional<PieceColor> winner_color = room_.match().state().winning_color()) {
            const Player* winner =
                *winner_color == PieceColor::White ? room_.white_player() : room_.black_player();
            const Player* loser =
                *winner_color == PieceColor::White ? room_.black_player() : room_.white_player();
            if (winner != nullptr) {
                player_repository_.update_rating(winner->id(), winner->rating() + 25);
                if (loser != nullptr) {
                    int loser_rating = loser->rating() - 25;
                    if (loser_rating < 0) {
                        loser_rating = 0;
                    }
                    player_repository_.update_rating(loser->id(), loser_rating);
                }
                game_repository_.finish_game(*game_id, winner->id());
            }
        }
    }

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

void GameServer::tick_once() {
    accept_new_clients();
    process_pending_logins();
    process_matchmaking_timeouts();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();

    if (room_.active()) {
        process_active_room(elapsed, last_tick_);
        if (room_.match().is_game_over()) {
            finish_active_room();
        }
    }

    prune_sessions();

    if (elapsed < kTargetFrameMs) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GameServer::run() {
    std::cout << "Server started\n";
    last_tick_ = std::chrono::steady_clock::now();

#ifdef KFC_TEST_BUILD
    while (!stop_requested_) {
#else
    while (true) {
#endif
        tick_once();
    }
}

}  // namespace kfc
