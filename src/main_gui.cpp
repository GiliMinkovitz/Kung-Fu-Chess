// Repository: https://github.com/GiliMinkovitz/Kung-Fu-Chess.git

#include "model/piece.h"
#include "engine/game_engine.h"
#include "model/board_model.h"
#include "model/game_config.h"
#include "network/game_redirect_message_reader.h"
#include "network/matchmaking_message_reader.h"
#include "network/network_game_redirect_flow.h"
#include "network/network_input_handler.h"
#include "network/network_login_session.h"
#include "network/snapshot_reader.h"
#include "network/websocket_client.h"
#include "ui/layout/board_layout.h"
#include "ui/rendering/ctd26_renderer.h"
#include "ui/rendering/i_ui_input_sink.h"
#include "ui/controller/ui_controller.h"

#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace {

constexpr std::uint16_t kServerPort = 8765;
constexpr std::size_t kDefaultBoardRows = 8;
constexpr std::size_t kDefaultBoardCols = 8;

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

bool is_network_mode(int argc, char* argv[]) {
    return argc >= 2 && std::string_view{argv[1]} == "--network";
}

[[nodiscard]] bool view_is_in_bounds(const kfc::BoardViewModel& view, std::size_t row,
                                     std::size_t col) noexcept {
    return row < view.height && col < view.width;
}

[[nodiscard]] bool view_is_selectable_piece(const kfc::BoardViewModel& view, std::size_t row,
                                            std::size_t col) {
    if (!kfc::board_view_piece_at(view, row, col).has_value()) {
        return false;
    }
    if (kfc::board_view_is_move_origin(view, row, col)) {
        return false;
    }
    if (kfc::board_view_is_jump_origin(view, row, col)) {
        return false;
    }
    return !kfc::board_view_is_resting_cell(view, row, col);
}

[[nodiscard]] bool view_is_friendly_to_selection(const kfc::BoardViewModel& view, std::size_t row,
                                                 std::size_t col) {
    if (!view.selection.has_value() || !view_is_selectable_piece(view, row, col)) {
        return false;
    }

    const auto [selected_row, selected_col] = *view.selection;
    const std::optional selected_piece = kfc::board_view_piece_at(view, selected_row, selected_col);
    const std::optional cell_piece = kfc::board_view_piece_at(view, row, col);
    if (!selected_piece.has_value() || !cell_piece.has_value()) {
        return false;
    }

    return selected_piece->color == cell_piece->color;
}

class LoginInputSink final : public kfc::IUiInputSink {
public:
    LoginInputSink(kfc::Ctd26Renderer& renderer, bool& play_requested)
        : renderer_{renderer}, play_requested_{play_requested} {}

    void on_pixel_click(int x, int y) override {
        const kfc::LoginScreenLayout layout = renderer_.login_screen_layout();
        if (x >= layout.button_x && x < layout.button_x + layout.button_w &&
            y >= layout.button_y && y < layout.button_y + layout.button_h) {
            play_requested_ = true;
        }
    }

    void on_pixel_jump(int /*x*/, int /*y*/) override {}

private:
    kfc::Ctd26Renderer& renderer_;
    bool& play_requested_;
};

class NetworkGuiInputSink final : public kfc::IUiInputSink {
public:
    NetworkGuiInputSink(kfc::NetworkInputHandler& input, const kfc::BoardViewModel& view,
                        kfc::BoardLayout layout)
        : input_{input}, view_{&view}, layout_{layout} {}

    void update_view(const kfc::BoardViewModel& view) noexcept { view_ = &view; }

    void set_layout(kfc::BoardLayout layout) noexcept { layout_ = layout; }

    void set_local_side(std::optional<kfc::PieceColor> side) noexcept { local_side_ = side; }

    void on_pixel_click(int x, int y) override { handle_click(x, y); }

    void on_pixel_jump(int x, int y) override { handle_jump(x, y); }

private:
    [[nodiscard]] bool pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const {
        if (view_->height == 0 || view_->width == 0) {
            return false;
        }

        return layout_.try_pixel_to_cell(x, y, view_->width, view_->height, row, col);
    }

    [[nodiscard]] bool is_actionable_piece(std::size_t row, std::size_t col) const {
        if (!view_is_selectable_piece(*view_, row, col)) {
            return false;
        }
        if (!local_side_.has_value()) {
            return true;
        }
        const std::optional piece = kfc::board_view_piece_at(*view_, row, col);
        return piece.has_value() && piece->color == *local_side_;
    }

    void handle_click(int x, int y) {
        if (view_->game_over) {
            return;
        }

        std::size_t row = 0;
        std::size_t col = 0;
        if (!pixel_to_cell(x, y, row, col) || !view_is_in_bounds(*view_, row, col)) {
            return;
        }

        if (!view_->selection.has_value()) {
            if (is_actionable_piece(row, col)) {
                input_.send_select(row, col);
            }
            return;
        }

        if (view_is_friendly_to_selection(*view_, row, col)) {
            handle_friendly_click(row, col);
            return;
        }

        handle_move_attempt(row, col);
    }

    void handle_friendly_click(std::size_t row, std::size_t col) {
        const auto [selected_row, selected_col] = *view_->selection;
        if (selected_row == row && selected_col == col) {
            if (is_actionable_piece(row, col)) {
                input_.send_jump(row, col);
                input_.send_clear();
            }
            return;
        }

        if (is_actionable_piece(row, col)) {
            input_.send_select(row, col);
        }
    }

    void handle_move_attempt(std::size_t row, std::size_t col) {
        if (!view_->selection.has_value()) {
            return;
        }

        const auto [from_row, from_col] = *view_->selection;
        if (kfc::board_view_is_move_origin(*view_, from_row, from_col) ||
            kfc::board_view_is_jump_origin(*view_, from_row, from_col) ||
            kfc::board_view_is_resting_cell(*view_, from_row, from_col)) {
            return;
        }

        input_.send_move(row, col);
    }

    void handle_jump(int x, int y) {
        if (view_->game_over) {
            return;
        }

        std::size_t row = 0;
        std::size_t col = 0;
        if (!pixel_to_cell(x, y, row, col) || !view_is_in_bounds(*view_, row, col)) {
            return;
        }

        if (!kfc::board_view_piece_at(*view_, row, col).has_value()) {
            return;
        }

        if (!is_actionable_piece(row, col)) {
            return;
        }

        if (kfc::board_view_is_move_origin(*view_, row, col) ||
            kfc::board_view_is_jump_origin(*view_, row, col) ||
            kfc::board_view_is_resting_cell(*view_, row, col)) {
            return;
        }

        input_.send_jump(row, col);
        input_.send_clear();
    }

    kfc::NetworkInputHandler& input_;
    const kfc::BoardViewModel* view_;
    kfc::BoardLayout layout_;
    std::optional<kfc::PieceColor> local_side_;
};

struct NetworkGuiState {
    kfc::MatchmakingState matchmaking{kfc::MatchmakingState::Idle};
    std::optional<kfc::PieceColor> local_side;
    std::optional<kfc::GameRedirectInfo> redirect;
    bool using_game_connection = false;
};

[[nodiscard]] std::optional<kfc::PieceColor> matchmaking_local_side(kfc::MatchmakingState state) {
    switch (state) {
        case kfc::MatchmakingState::MatchedWhite:
        case kfc::MatchmakingState::GameStartingWhite:
            return kfc::PieceColor::White;
        case kfc::MatchmakingState::MatchedBlack:
        case kfc::MatchmakingState::GameStartingBlack:
            return kfc::PieceColor::Black;
        case kfc::MatchmakingState::Idle:
        case kfc::MatchmakingState::Searching:
        case kfc::MatchmakingState::Playing:
        case kfc::MatchmakingState::Timeout:
            return std::nullopt;
    }
    return std::nullopt;
}

void handle_network_message(const std::string& message,
                            std::optional<kfc::BoardViewModel>& latest_view,
                            NetworkGuiState& gui_state);

void drain_network_messages(kfc::WebSocketClient& client, std::optional<kfc::BoardViewModel>& latest_view,
                            NetworkGuiState& gui_state) {
    while (const std::optional<std::string> message = client.try_receive_snapshot()) {
        handle_network_message(*message, latest_view, gui_state);
    }
}

[[nodiscard]] std::optional<std::string_view> matchmaking_overlay_text(
    kfc::MatchmakingState state) {
    switch (state) {
        case kfc::MatchmakingState::Idle:
            return std::nullopt;
        case kfc::MatchmakingState::Searching:
            return "Searching for opponent...";
        case kfc::MatchmakingState::MatchedWhite:
            return "Match found - You are White";
        case kfc::MatchmakingState::MatchedBlack:
            return "Match found - You are Black";
        case kfc::MatchmakingState::GameStartingWhite:
            return "Game starting - You are White";
        case kfc::MatchmakingState::GameStartingBlack:
            return "Game starting - You are Black";
        case kfc::MatchmakingState::Playing:
            return std::nullopt;
        case kfc::MatchmakingState::Timeout:
            return "No players available";
    }
    return std::nullopt;
}

[[nodiscard]] kfc::BoardViewModel empty_waiting_board_view() {
    kfc::BoardViewModel view;
    view.height = kDefaultBoardRows;
    view.width = kDefaultBoardCols;
    view.cells.assign(kDefaultBoardRows * kDefaultBoardCols, kfc::CellView{});
    return view;
}

void handle_network_message(const std::string& message,
                            std::optional<kfc::BoardViewModel>& latest_view,
                            NetworkGuiState& gui_state) {
    if (const std::optional<kfc::BoardViewModel> view = kfc::read_snapshot(message)) {
        latest_view = view;
        gui_state.matchmaking = kfc::MatchmakingState::Playing;
        return;
    }

    if (const std::optional<kfc::MatchmakingState> matchmaking =
            kfc::read_matchmaking_message(message)) {
        gui_state.matchmaking = *matchmaking;
        if (const std::optional<kfc::PieceColor> side = matchmaking_local_side(*matchmaking)) {
            gui_state.local_side = side;
        }
        return;
    }

    if (const std::optional<kfc::GameRedirectInfo> redirect =
            kfc::read_game_redirect_message(message)) {
        gui_state.redirect = redirect;
        if (!gui_state.local_side.has_value()) {
            gui_state.local_side = redirect->side;
        }
    }
}

int run_offline_gui() {
    kfc::GameEngine engine(default_board());
    kfc::UiController controller(engine.state(), std::make_unique<kfc::Ctd26Renderer>());

    auto last_frame = std::chrono::steady_clock::now();

    while (true) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!controller.frame(elapsed).should_continue) {
                break;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    controller.shutdown();
    return 0;
}

[[nodiscard]] bool wait_for_login_handshake(kfc::WebSocketClient& client,
                                            kfc::NetworkLoginSession& login_session,
                                            kfc::Ctd26Renderer& renderer,
                                            kfc::UiController& controller,
                                            std::string& username) {
    auto last_frame = std::chrono::steady_clock::now();
    while (login_session.phase() == kfc::NetworkLoginPhase::LoginSent) {
        if (const std::optional<std::string> message = client.try_receive_snapshot()) {
            login_session.handle_message(*message);
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!renderer.present_login_screen(username).should_continue) {
                controller.shutdown();
                client.disconnect();
                return false;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    if (login_session.is_login_failed()) {
        std::cerr << "Login failed: " << login_session.login_failure_reason() << '\n';
        return false;
    }

    if (!login_session.try_send_play()) {
        std::cerr << "Failed to send play after login\n";
        return false;
    }

    return true;
}

[[nodiscard]] bool maybe_switch_to_game_connection(kfc::WebSocketClient& lobby_client,
                                                   kfc::WebSocketClient& game_client,
                                                   kfc::NetworkInputHandler& game_input,
                                                   NetworkGuiState& gui_state) {
    if (!gui_state.redirect.has_value() || gui_state.using_game_connection) {
        return false;
    }

    kfc::NetworkGameRedirectFlow redirect_flow{lobby_client, game_client, game_input};
    if (!redirect_flow.execute(*gui_state.redirect)) {
        return false;
    }

    gui_state.using_game_connection = true;
    return true;
}

int run_network_gui() {
    kfc::WebSocketClient lobby_client("127.0.0.1", kServerPort);
    kfc::WebSocketClient game_client("127.0.0.1", kServerPort);
    lobby_client.connect();

    auto renderer = std::make_unique<kfc::Ctd26Renderer>();
    kfc::Ctd26Renderer* renderer_ptr = renderer.get();
    kfc::UiController controller(kDefaultBoardRows, kDefaultBoardCols, std::move(renderer));
    renderer_ptr->attach_input_sink(nullptr);

    kfc::NetworkInputHandler lobby_input(lobby_client);
    kfc::NetworkInputHandler game_input(game_client);
    kfc::NetworkLoginSession login_session(lobby_input);

    std::string username;
    bool play_requested = false;
    LoginInputSink login_sink(*renderer_ptr, play_requested);
    renderer_ptr->attach_input_sink(&login_sink);

    auto last_frame = std::chrono::steady_clock::now();
    while (true) {
        play_requested = false;
        last_frame = std::chrono::steady_clock::now();
        while (!play_requested) {
            const auto now = std::chrono::steady_clock::now();
            const auto elapsed =
                std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
            if (elapsed >= kfc::kTargetFrameMs) {
                if (!renderer_ptr->present_login_screen(username).should_continue) {
                    controller.shutdown();
                    lobby_client.disconnect();
                    return 0;
                }
                last_frame = now;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        renderer_ptr->attach_input_sink(nullptr);

        if (!login_session.send_login(username)) {
            std::cerr << "Failed to send login\n";
            login_session.reset();
            renderer_ptr->attach_input_sink(&login_sink);
            continue;
        }

        if (!wait_for_login_handshake(lobby_client, login_session, *renderer_ptr, controller,
                                      username)) {
            if (!lobby_client.is_connected()) {
                return 0;
            }
            login_session.reset();
            renderer_ptr->attach_input_sink(&login_sink);
            continue;
        }

        break;
    }

    std::optional<kfc::BoardViewModel> latest_view;
    NetworkGuiState gui_state;
    const kfc::BoardViewModel waiting_board_view = empty_waiting_board_view();
    last_frame = std::chrono::steady_clock::now();
    while (!latest_view.has_value()) {
        drain_network_messages(lobby_client, latest_view, gui_state);
        if (maybe_switch_to_game_connection(lobby_client, game_client, game_input, gui_state)) {
            drain_network_messages(game_client, latest_view, gui_state);
        } else if (gui_state.using_game_connection) {
            drain_network_messages(game_client, latest_view, gui_state);
        }

        renderer_ptr->set_overlay_text(matchmaking_overlay_text(gui_state.matchmaking));

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!controller.present(waiting_board_view).should_continue) {
                controller.shutdown();
                lobby_client.disconnect();
                game_client.disconnect();
                return 0;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    renderer_ptr->set_overlay_text(std::nullopt);

    NetworkGuiInputSink input_sink(game_input, *latest_view, renderer_ptr->board_layout());
    renderer_ptr->attach_input_sink(&input_sink);

    last_frame = std::chrono::steady_clock::now();

    while (true) {
        if (gui_state.using_game_connection) {
            drain_network_messages(game_client, latest_view, gui_state);
        } else {
            drain_network_messages(lobby_client, latest_view, gui_state);
            if (maybe_switch_to_game_connection(lobby_client, game_client, game_input, gui_state)) {
                drain_network_messages(game_client, latest_view, gui_state);
            }
        }

        input_sink.update_view(*latest_view);
        input_sink.set_layout(renderer_ptr->board_layout());
        input_sink.set_local_side(gui_state.local_side);

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!controller.present(*latest_view).should_continue) {
                break;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    controller.shutdown();
    lobby_client.disconnect();
    game_client.disconnect();
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        if (is_network_mode(argc, argv)) {
            return run_network_gui();
        }
        return run_offline_gui();
    } catch (const std::exception& ex) {
        std::cerr << "GUI error: " << ex.what() << '\n';
        return 1;
    }
}
