#pragma once

#include "../logic/game_state.h"
#include "ui/layout/board_layout.h"

#include <iosfwd>
#include <string>

namespace kfc {

namespace test {
struct GameInputHandlerTestAccess;
}

// Input adapter: maps VPL text commands and pixel coordinates to GameState actions.
// Encodes click semantics (select, move, jump-on-friendly-reclick) but does not
// validate moves or mutate the board directly.
class GameInputHandler {
    friend struct test::GameInputHandlerTestAccess;

public:
    explicit GameInputHandler(GameState& state);

    void set_board_layout(BoardLayout layout);
    void execute(const std::string& command, std::ostream& out);
    // Shared entry points used by both VPL parsing and the GUI (UiController).
    void handle_pixel_click(int x, int y);
    void handle_pixel_jump(int x, int y);

private:
    void bootstrap_default_layout();

    GameState& state_;
    BoardLayout layout_{};

    void handle_click(int x, int y);
    void handle_select(std::size_t row, std::size_t col);
    void handle_friendly_click(std::size_t row, std::size_t col);
    void handle_move_attempt(std::size_t row, std::size_t col);
    void handle_jump(int x, int y);
    void handle_wait(std::int64_t ms);
    void handle_print_board(std::ostream& out);

    [[nodiscard]] bool pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const;
};

}  // namespace kfc
