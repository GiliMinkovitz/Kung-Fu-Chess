#include "server/rating_service.h"
#include "server/game_server.h"

#include "model/game_config.h"
#include "server/game_message_parser.h"
#include "server/game_result_message_writer.h"
#include "server/snapshot_writer.h"
#include "ui/view/board_view_builder.h"

#include <chrono>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <thread>

namespace {

[[nodiscard]] bool is_room_player_connected(const kfc::PlayerSession* session) {
    return session != nullptr && session->connection() != nullptr &&
           session->connection()->is_open();
}

}  // namespace

namespace kfc {

GameServer::GameServer(unsigned short port, BoardModel default_board, const std::string& db_path)
    : websocket_server_(port),
      room_(std::move(default_board)),
      database_(db_path),
      player_repository_(database_),
      game_repository_(database_),
      authentication_service_(player_repository_) {
    if (!database_.open() || !database_.initialize_schema()) {
        throw std::runtime_error("Failed to initialize database");
    }
    last_tick_ = std::chrono::steady_clock::now();
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
                if (const auto request = parse_login_message(*raw_message)) {
                    if (session_registry_.is_online(request->username)) {
                        session.connection()->try_send("login_failed already_connected");
                        continue;
                    }

                    const AuthenticationResult auth =
                        authentication_service_.authenticate(request->username, request->password);
                    if (!auth.success) {
                        session.connection()->try_send("login_failed " + auth.failure_reason);
                        continue;
                    }

                    session.bind_player(*auth.player);
                    session_registry_.register_session(auth.player->username());
                    session.connection()->try_send("login_ok " + std::to_string(auth.player->rating()));
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
            if (it->has_player()) {
                session_registry_.unregister_session(it->player().username());
            }
            matchmaking_.remove(*it);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameServer::probe_active_room_connections() {
    if (PlayerSession* white = room_.white_session()) {
        if (ClientConnection* connection = white->connection()) {
            connection->probe_disconnect();
        }
    }
    if (PlayerSession* black = room_.black_session()) {
        if (ClientConnection* connection = black->connection()) {
            connection->probe_disconnect();
        }
    }
}

std::optional<PieceColor> GameServer::disconnected_player_color() const {
    const PlayerSession* white = room_.white_session();
    const PlayerSession* black = room_.black_session();
    if (white == nullptr || black == nullptr) {
        return std::nullopt;
    }

    const bool white_connected = is_room_player_connected(white);
    const bool black_connected = is_room_player_connected(black);
    if (white_connected && black_connected) {
        return std::nullopt;
    }
    if (!white_connected && black_connected) {
        return PieceColor::White;
    }
    if (white_connected && !black_connected) {
        return PieceColor::Black;
    }
    return std::nullopt;
}

bool GameServer::both_room_players_disconnected() const {
    return !is_room_player_connected(room_.white_session()) &&
           !is_room_player_connected(room_.black_session());
}

void GameServer::process_room_player_messages(PlayerSession& session, Match& match) {
    if (const auto raw_message = session.connection()->try_read()) {
        if (parse_resign_message(*raw_message)) {
            if (room_.contains(&session) && session.has_side()) {
                const PieceColor winner =
                    session.side() == PieceColor::White ? PieceColor::Black : PieceColor::White;
                finish_active_room(winner, FinishReason::Resign);
            }
            return;
        }

        if (const auto action = parse_message(*raw_message)) {
            if (is_action_allowed(session, match, *action)) {
                match.submit_action(*action);
            }
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
        if (is_room_player_connected(room_.white_session())) {
            room_.white_session()->connection()->try_send(snapshot);
        }
        if (is_room_player_connected(room_.black_session())) {
            room_.black_session()->connection()->try_send(snapshot);
        }

        last_tick = std::chrono::steady_clock::now();
    }

    if (!match.is_game_over()) {
        process_room_player_messages(*room_.white_session(), match);
        if (room_.active() && !match.is_game_over()) {
            process_room_player_messages(*room_.black_session(), match);
        }
    }
}

void GameServer::finish_active_room(std::optional<PieceColor> winner_color, FinishReason reason) {
    PlayerSession* white_session = room_.white_session();
    PlayerSession* black_session = room_.black_session();
    ClientConnection* white_connection =
        white_session != nullptr ? white_session->connection() : nullptr;
    ClientConnection* black_connection =
        black_session != nullptr ? black_session->connection() : nullptr;

    std::optional<RatingChange> rating_change;
    if (const std::optional<int> game_id = room_.db_game_id()) {
        if (winner_color.has_value()) {
            rating_change = update_ratings_for_result(*winner_color, *game_id);
        } else if (reason == FinishReason::Disconnect) {
            game_repository_.finish_game_without_winner(*game_id);
        }
    }

    if (winner_color.has_value() && rating_change.has_value() && white_connection != nullptr &&
        black_connection != nullptr) {
        const bool white_won = *winner_color == PieceColor::White;
        const std::string white_message = create_game_result_message(
            white_won, reason,
            white_won ? rating_change->winner_new_rating : rating_change->loser_new_rating);
        const std::string black_message = create_game_result_message(
            !white_won, reason,
            white_won ? rating_change->loser_new_rating : rating_change->winner_new_rating);

        (void)white_connection->send_message(white_message);
        (void)black_connection->send_message(black_message);
    }

    cleanup_finished_room();
}

const Player* GameServer::find_player_by_color(PieceColor color) const {
    return color == PieceColor::White ? room_.white_player() : room_.black_player();
}

std::optional<RatingChange> GameServer::update_ratings_for_result(PieceColor winner_color,
                                                                  int game_id) {
    const Player* winner = find_player_by_color(winner_color);
    const PieceColor loser_color =
        winner_color == PieceColor::White ? PieceColor::Black : PieceColor::White;
    const Player* loser = find_player_by_color(loser_color);
    if (winner == nullptr || loser == nullptr) {
        return std::nullopt;
    }

    const RatingChange change = rating_service_.calculate(winner->rating(), loser->rating());
    player_repository_.update_rating(winner->id(), change.winner_new_rating);
    player_repository_.update_rating(loser->id(), change.loser_new_rating);
    game_repository_.finish_game(game_id, winner->id());
    return change;
}

void GameServer::cleanup_finished_room() {
    PlayerSession* white = room_.white_session();
    PlayerSession* black = room_.black_session();

    room_.reset();

    if (white != nullptr) {
        refresh_session_player(*white);
        session_registry_.unregister_session(white->player().username());
    }
    if (black != nullptr) {
        refresh_session_player(*black);
        session_registry_.unregister_session(black->player().username());
    }
}

#ifdef KFC_TEST_BUILD
void GameServer::finish_active_room_for_tests(std::optional<PieceColor> winner_color,
                                              FinishReason reason) {
    finish_active_room(winner_color, reason);
}
#endif

void GameServer::refresh_session_player(PlayerSession& session) {
    if (!session.has_player()) {
        return;
    }
    if (const auto updated = player_repository_.find_by_username(session.player().username())) {
        session.bind_player(*updated);
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
        probe_active_room_connections();

        if (both_room_players_disconnected()) {
            finish_active_room(std::nullopt, FinishReason::Disconnect);
        } else if (const std::optional<PieceColor> disconnected = disconnected_player_color()) {
            const PieceColor winner =
                *disconnected == PieceColor::White ? PieceColor::Black : PieceColor::White;
            finish_active_room(winner, FinishReason::Disconnect);
        } else {
            process_active_room(elapsed, last_tick_);
            if (room_.active() && room_.match().is_game_over()) {
                finish_active_room(room_.match().state().winning_color(),
                                   FinishReason::KingCapture);
            }
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
