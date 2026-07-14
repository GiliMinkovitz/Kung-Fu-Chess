#pragma once

#include "../model/piece.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace kfc {

struct CellView {
    std::size_t row = 0;
    std::size_t col = 0;
    std::string token;
};

struct ActiveMoveView {
    Piece::Id piece_id = Piece::kInvalidId;
    std::size_t from_row = 0;
    std::size_t from_col = 0;
    std::size_t to_row = 0;
    std::size_t to_col = 0;
    float progress = 0.0f;
};

struct ActiveJumpView {
    Piece::Id piece_id = Piece::kInvalidId;
    std::size_t row = 0;
    std::size_t col = 0;
    float progress = 0.0f;
};

struct BoardViewModel {
    std::size_t rows = 0;
    std::size_t cols = 0;
    std::int64_t clock_ms = 0;
    bool game_over = false;
    std::optional<std::pair<std::size_t, std::size_t>> selection;
    std::vector<CellView> cells;
    std::vector<ActiveMoveView> active_moves;
    std::vector<ActiveJumpView> active_jumps;
};

class IUiRenderer {
public:
    virtual ~IUiRenderer() = default;
    virtual void init(std::size_t rows, std::size_t cols, int cell_pixel_size) = 0;
    virtual void render(const BoardViewModel& view) = 0;
    virtual void shutdown() = 0;
};

}  // namespace kfc
