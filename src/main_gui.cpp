// Repository: https://github.com/GiliMinkovitz/Kung-Fu-Chess.git

#include "engine/game_engine.h"
#include "model/board_model.h"
#include "model/game_config.h"
#include "network/matchmaking_message_reader.h"
#include "network/network_input_handler.h"
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

    void on_pixel_click(int x, int y) override { handle_click(x, y); }

    void on_pixel_jump(int x, int y) override { handle_jump(x, y); }

private:
    [[nodiscard]] bool pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const {
        if (view_->height == 0 || view_->width == 0) {
            return false;
        }

        return layout_.try_pixel_to_cell(x, y, view_->width, view_->height, row, col);
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
            if (view_is_selectable_piece(*view_, row, col)) {
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
            if (view_is_selectable_piece(*view_, row, col)) {
                input_.send_jump(row, col);
                input_.send_clear();
            }
            return;
        }

        input_.send_select(row, col);
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
};

struct NetworkGuiState {
    kfc::MatchmakingState matchmaking{kfc::MatchmakingState::Idle};
};

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

int run_network_gui() {
    kfc::WebSocketClient client("127.0.0.1", kServerPort);
    client.connect();

    auto renderer = std::make_unique<kfc::Ctd26Renderer>();
    kfc::Ctd26Renderer* renderer_ptr = renderer.get();
    kfc::UiController controller(kDefaultBoardRows, kDefaultBoardCols, std::move(renderer));

    std::string username;
    bool play_requested = false;
    LoginInputSink login_sink(*renderer_ptr, play_requested);
    renderer_ptr->attach_input_sink(&login_sink);

    auto last_frame = std::chrono::steady_clock::now();
    while (!play_requested) {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!renderer_ptr->present_login_screen(username).should_continue) {
                controller.shutdown();
                client.disconnect();
                return 0;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    renderer_ptr->attach_input_sink(nullptr);

    const std::string login_name = username.empty() ? "Player1" : username;
    kfc::NetworkInputHandler login_handler(client);
    login_handler.send_login(login_name);
    login_handler.send_play();

    std::optional<kfc::BoardViewModel> latest_view;
    NetworkGuiState gui_state;
    const kfc::BoardViewModel waiting_board_view = empty_waiting_board_view();
    last_frame = std::chrono::steady_clock::now();
    while (!latest_view.has_value()) {
        if (const std::optional<std::string> message = client.try_receive_snapshot()) {
            handle_network_message(*message, latest_view, gui_state);
        }

        renderer_ptr->set_overlay_text(matchmaking_overlay_text(gui_state.matchmaking));

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - last_frame).count();
        if (elapsed >= kfc::kTargetFrameMs) {
            if (!controller.present(waiting_board_view).should_continue) {
                controller.shutdown();
                client.disconnect();
                return 0;
            }
            last_frame = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    renderer_ptr->set_overlay_text(std::nullopt);

    kfc::NetworkInputHandler network_input(client);
    NetworkGuiInputSink input_sink(network_input, *latest_view, renderer_ptr->board_layout());
    renderer_ptr->attach_input_sink(&input_sink);

    last_frame = std::chrono::steady_clock::now();

    while (true) {
        if (const std::optional<std::string> message = client.try_receive_snapshot()) {
            handle_network_message(*message, latest_view, gui_state);
        }

        input_sink.update_view(*latest_view);
        input_sink.set_layout(renderer_ptr->board_layout());

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
    client.disconnect();
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
