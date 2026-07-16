#pragma once

#include "model/piece.h"
#include "realtime/render_snapshot.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace kfc {

struct PieceView {
    PieceKind kind = PieceKind::Pawn;
    PieceColor color = PieceColor::White;
};

struct CellView {
    std::optional<PieceView> piece;
};

struct BoardViewModel {
    std::size_t height = 0;
    std::size_t width = 0;
    std::int64_t clock_ms = 0;
    bool game_over = false;
    std::optional<std::pair<std::size_t, std::size_t>> selection;
    std::vector<CellView> cells;
    AnimationSnapshot animations;
};

[[nodiscard]] std::size_t board_view_cell_index(std::size_t row, std::size_t col,
                                                std::size_t width) noexcept;

[[nodiscard]] const CellView& board_view_cell_at(const BoardViewModel& view, std::size_t row,
                                                 std::size_t col);

[[nodiscard]] std::optional<PieceView> board_view_piece_at(const BoardViewModel& view, std::size_t row,
                                                           std::size_t col);

[[nodiscard]] bool board_view_is_move_origin(const BoardViewModel& view, std::size_t row,
                                             std::size_t col);

[[nodiscard]] bool board_view_is_jumping_cell(const BoardViewModel& view, std::size_t row,
                                              std::size_t col);

[[nodiscard]] float board_view_jump_progress_at(const BoardViewModel& view, std::size_t row,
                                                std::size_t col);

[[nodiscard]] bool board_view_is_resting_cell(const BoardViewModel& view, std::size_t row,
                                              std::size_t col);

[[nodiscard]] float board_view_rest_progress_at(const BoardViewModel& view, std::size_t row,
                                                 std::size_t col);

[[nodiscard]] RestKind board_view_rest_kind_at(const BoardViewModel& view, std::size_t row,
                                               std::size_t col);

}  // namespace kfc
