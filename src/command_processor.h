#pragma once

#include "game_state.h"

#include <iosfwd>
#include <string>

namespace kfc {

class CommandProcessor {
public:
    explicit CommandProcessor(GameState& state);

    void execute(const std::string& command, std::ostream& out);

private:
    static constexpr int kCellPixelSize = 100;

    GameState& state_;

    void handle_click(int x, int y);
    void handle_jump(int x, int y);
    void handle_wait(std::int64_t ms);
    void handle_print_board(std::ostream& out);

    [[nodiscard]] bool pixel_to_cell(int x, int y, std::size_t& row, std::size_t& col) const;
};

}  // namespace kfc
