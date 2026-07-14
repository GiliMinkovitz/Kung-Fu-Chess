#pragma once

#include "../logic/game_state.h"
#include "../model/game_config.h"

#include <iosfwd>
#include <string>

namespace kfc {

class CommandProcessor {
public:
    explicit CommandProcessor(GameState& state, int cell_pixel_size = kCellPixelSize);

    void execute(const std::string& command, std::ostream& out);
    void handle_pixel_click(int x, int y);
    void handle_pixel_jump(int x, int y);

private:
    GameState& state_;
    int cell_pixel_size_ = kCellPixelSize;

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
