#include "server/rating_service.h"
#include "server/game_server.h"

#include "model/game_config.h"
#include "server/game_message_parser.h"
#include "server/game_result_message_writer.h"

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

GameServer::GameServer(unsigned short port, BoardModel default_board,
                       app::GameServerDependencies dependencies)
    : websocket_server_(port),
      matchmaking_service_(*this),
      room_manager_(std::move(default_board)),
      database_(dependencies.database),
      user_repository_(dependencies.user_repository),
      game_repository_(dependencies.game_repository),
      authentication_service_(dependencies.authentication_service) {
    last_tick_ = std::chrono::steady_clock::now();
}

GameServer::~GameServer() = default;

WebSocketServer& GameServer::websocket_server() noexcept {
    return websocket_server_;
}

MatchmakingService& GameServer::matchmaking_service() noexcept {
    return matchmaking_service_;
}

RoomId GameServer::create_match(PlayerSession* white, PlayerSession* black) {
    const RoomId room_id = room_manager_.create_room();
    last_room_id_ = room_id;

    Room* room = room_manager_.find_room(room_id);
    room->activate(&white->player(), &black->player());

    RoomContext context;
    context.white_session = white;
    context.black_session = black;
    context.db_game_id =
        game_repository_.create_game(white->player().id(), black->player().id());
    room_contexts_[room_id] = context;

    return room_id;
}

void GameServer::notify_match_created(const MatchCreated& match) {
    match.white->connection()->try_send("match_found white");
    match.black->connection()->try_send("match_found black");
    match.white->connection()->try_send("game_start white");
    match.black->connection()->try_send("game_start black");
}

RoomManager& GameServer::room_manager() noexcept {
    return room_manager_;
}

Room& GameServer::room() noexcept {
    const std::vector<Room*> active = room_manager_.active_rooms();
    if (!active.empty()) {
        return *active.front();
    }
    if (last_room_id_.has_value()) {
        return *room_manager_.find_room(*last_room_id_);
    }
    const RoomId room_id = room_manager_.create_room();
    last_room_id_ = room_id;
    return *room_manager_.find_room(room_id);
}

std::optional<int> GameServer::room_db_game_id() const noexcept {
    for (const Room* active_room : room_manager_.active_rooms()) {
        if (const RoomContext* context = find_context(active_room->id())) {
            return context->db_game_id;
        }
    }
    return std::nullopt;
}

IDatabaseConnection& GameServer::database() noexcept {
    return database_;
}

IGameRepository& GameServer::game_repository() noexcept {
    return game_repository_;
}

IUserRepository& GameServer::user_repository() noexcept {
    return user_repository_;
}

GameServer::RoomContext* GameServer::find_context(RoomId room_id) {
    const auto it = room_contexts_.find(room_id);
    return it != room_contexts_.end() ? &it->second : nullptr;
}

const GameServer::RoomContext* GameServer::find_context(RoomId room_id) const {
    const auto it = room_contexts_.find(room_id);
    return it != room_contexts_.end() ? &it->second : nullptr;
}

Room* GameServer::find_session_room(const PlayerSession& session) {
    if (!session.has_room()) {
        return nullptr;
    }
    return room_manager_.find_room(session.room_id());
}

void GameServer::bind_authenticated_user(PlayerSession& session,
                                         const Player& authenticated_player) {
    session.assign_user(static_cast<UserId>(authenticated_player.id()), authenticated_player.username(),
                        authenticated_player.rating());
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

            if (!session.has_user()) {
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

                    bind_authenticated_user(session, *auth.player);
                    session_registry_.register_session(auth.player->username());
                    session.connection()->try_send("login_ok " + std::to_string(auth.player->rating()));
                }
                continue;
            }

            if (parse_play_message(*raw_message)) {
                session.request_play();
                if (session.state() == PlayerSessionState::Searching) {
                    const auto now = std::chrono::steady_clock::now();
                    if (const auto match = matchmaking_service_.enqueue(session, now)) {
                        notify_match_created(*match);
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
    for (PlayerSession* session : matchmaking_service_.check_timeouts(now)) {
#ifndef KFC_TEST_BUILD
        std::cout << "Matchmaking timeout for session " << session->id() << " (player "
                  << session->player().username() << ")\n";
#endif
        session->connection()->try_send("search_timeout");
        session->cancel_search();
    }
}

void GameServer::prune_sessions() {
    websocket_server_.prune_disconnected();

    for (auto it = sessions_.begin(); it != sessions_.end();) {
        if (!it->connection()->is_open()) {
            if (it->has_user()) {
                session_registry_.unregister_session(it->player().username());
            }
            matchmaking_service_.remove(*it);
            it = sessions_.erase(it);
        } else {
            ++it;
        }
    }
}

void GameServer::probe_room_connections(const RoomContext& context) {
    if (context.white_session != nullptr) {
        if (ClientConnection* connection = context.white_session->connection()) {
            connection->probe_disconnect();
        }
    }
    if (context.black_session != nullptr) {
        if (ClientConnection* connection = context.black_session->connection()) {
            connection->probe_disconnect();
        }
    }
}

std::optional<PieceColor> GameServer::disconnected_player_color(const RoomContext& context) const {
    if (context.white_session == nullptr || context.black_session == nullptr) {
        return std::nullopt;
    }

    const bool white_connected = is_room_player_connected(context.white_session);
    const bool black_connected = is_room_player_connected(context.black_session);
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

bool GameServer::both_room_players_disconnected(const RoomContext& context) const {
    return !is_room_player_connected(context.white_session) &&
           !is_room_player_connected(context.black_session);
}

void GameServer::process_room_player_messages(Room& room, PlayerSession& session) {
    if (const auto raw_message = session.connection()->try_read()) {
        if (parse_resign_message(*raw_message)) {
            if (room.contains_player(&session.player()) && session.has_side()) {
                const PieceColor winner =
                    session.side() == PieceColor::White ? PieceColor::Black : PieceColor::White;
                finish_room(room.id(), winner, FinishReason::Resign);
            }
            return;
        }

        if (const auto action = parse_message(*raw_message)) {
            if (is_action_allowed(session, room.match(), *action)) {
                room.submit_action(*action);
            }
        }
    }
}

void GameServer::process_playing_session_messages() {
    for (PlayerSession& session : sessions_) {
        if (session.state() != PlayerSessionState::Playing || !session.has_room()) {
            continue;
        }

        Room* room = find_session_room(session);
        if (room == nullptr || !room->active() || room->is_game_over()) {
            continue;
        }

        process_room_player_messages(*room, session);
    }
}

void GameServer::process_active_rooms(std::int64_t elapsed,
                                      std::chrono::steady_clock::time_point& last_tick) {
    for (Room* room : room_manager_.active_rooms()) {
        RoomContext* context = find_context(room->id());
        if (context == nullptr) {
            continue;
        }

        if (elapsed >= kTargetFrameMs && !room->is_game_over()) {
            room->tick(elapsed);

            const std::string snapshot = room->generate_snapshot();
            if (is_room_player_connected(context->white_session)) {
                context->white_session->connection()->try_send(snapshot);
            }
            if (is_room_player_connected(context->black_session)) {
                context->black_session->connection()->try_send(snapshot);
            }

            last_tick = std::chrono::steady_clock::now();
        }
    }

    process_playing_session_messages();
}

void GameServer::finish_room(RoomId room_id, std::optional<PieceColor> winner_color,
                             FinishReason reason) {
    Room* room = room_manager_.find_room(room_id);
    RoomContext* context = find_context(room_id);
    if (room == nullptr || context == nullptr) {
        return;
    }

    PlayerSession* white_session = context->white_session;
    PlayerSession* black_session = context->black_session;
    ClientConnection* white_connection =
        white_session != nullptr ? white_session->connection() : nullptr;
    ClientConnection* black_connection =
        black_session != nullptr ? black_session->connection() : nullptr;

    std::optional<RatingChange> rating_change;
    if (context->db_game_id.has_value()) {
        if (winner_color.has_value()) {
            rating_change = update_ratings_for_result(*room, *winner_color, *context->db_game_id);
        } else if (reason == FinishReason::Disconnect) {
            game_repository_.finish_game_without_winner(*context->db_game_id);
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

    cleanup_finished_room(room_id);
}

const Player* GameServer::find_player_by_color(const Room& room, PieceColor color) const {
    return color == PieceColor::White ? room.white_player() : room.black_player();
}

std::optional<RatingChange> GameServer::update_ratings_for_result(const Room& room,
                                                                  PieceColor winner_color,
                                                                  int game_id) {
    const Player* winner = find_player_by_color(room, winner_color);
    const PieceColor loser_color =
        winner_color == PieceColor::White ? PieceColor::Black : PieceColor::White;
    const Player* loser = find_player_by_color(room, loser_color);
    if (winner == nullptr || loser == nullptr) {
        return std::nullopt;
    }

    const RatingChange change = rating_service_.calculate(winner->rating(), loser->rating());
    user_repository_.update_rating(static_cast<UserId>(winner->id()), change.winner_new_rating);
    user_repository_.update_rating(static_cast<UserId>(loser->id()), change.loser_new_rating);
    game_repository_.finish_game(game_id, winner->id());
    return change;
}

void GameServer::cleanup_finished_room(RoomId room_id) {
    Room* room = room_manager_.find_room(room_id);
    RoomContext* context = find_context(room_id);
    if (room == nullptr || context == nullptr) {
        return;
    }

    PlayerSession* white = context->white_session;
    PlayerSession* black = context->black_session;

    room->reset();
    last_room_id_ = room_id;

    if (white != nullptr) {
        white->clear_side();
        white->clear_room();
        refresh_session_player(*white);
        session_registry_.unregister_session(white->player().username());
    }
    if (black != nullptr) {
        black->clear_side();
        black->clear_room();
        refresh_session_player(*black);
        session_registry_.unregister_session(black->player().username());
    }

    room_contexts_.erase(room_id);
}

#ifdef KFC_TEST_BUILD
void GameServer::finish_active_room_for_tests(std::optional<PieceColor> winner_color,
                                              FinishReason reason) {
    const std::vector<Room*> active = room_manager_.active_rooms();
    if (active.empty()) {
        return;
    }
    finish_room(active.front()->id(), winner_color, reason);
}
#endif

void GameServer::refresh_session_player(PlayerSession& session) {
    if (!session.has_user()) {
        return;
    }
    if (const auto updated = user_repository_.find_profile_by_id(session.user_id())) {
        session.assign_user(session.user_id(), updated->username(), updated->rating());
    }
}

void GameServer::tick_once() {
    accept_new_clients();
    process_pending_logins();
    process_matchmaking_timeouts();

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - last_tick_).count();

    for (Room* room : room_manager_.active_rooms()) {
        const RoomContext* context = find_context(room->id());
        if (context == nullptr) {
            continue;
        }

        probe_room_connections(*context);

        if (both_room_players_disconnected(*context)) {
            finish_room(room->id(), std::nullopt, FinishReason::Disconnect);
        } else if (const std::optional<PieceColor> disconnected =
                       disconnected_player_color(*context)) {
            const PieceColor winner =
                *disconnected == PieceColor::White ? PieceColor::Black : PieceColor::White;
            finish_room(room->id(), winner, FinishReason::Disconnect);
        }
    }

    if (!room_manager_.active_rooms().empty()) {
        process_active_rooms(elapsed, last_tick_);

        for (Room* room : room_manager_.active_rooms()) {
            if (room->is_game_over()) {
                finish_room(room->id(), room->match().state().winning_color(),
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
#ifndef KFC_TEST_BUILD
    std::cout << "Server started\n";
#endif
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
