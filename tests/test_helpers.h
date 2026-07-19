#pragma once

#include "model/board_model.h"
#include "model/piece.h"
#include "model/piece_factory.h"
#include "model/position.h"
#include "realtime/collision_resolver.h"
#include "io/game_input_handler.h"
#include "logic/game_state.h"
#include "realtime/move_scheduler.h"

#include <cassert>
#include <string>
#include <vector>

namespace kfc::test {

struct BoardModelTestAccess {
    static void set_cell_piece_id(BoardModel& board, std::size_t row, std::size_t col,
                                  Piece::Id id) {
        board.cells_[row][col] = id;
    }

    static const Piece* find_piece_by_id(const BoardModel& board, Piece::Id id) {
        return board.find_piece_by_id(id);
    }

    static Piece* find_piece_by_id(BoardModel& board, Piece::Id id) {
        return board.find_piece_by_id(id);
    }
};

struct GameInputHandlerTestAccess {
    static void handle_move_attempt(GameInputHandler& handler, std::size_t row, std::size_t col) {
        handler.handle_move_attempt(row, col);
    }
};

struct GameStateTestAccess {
    static bool is_legal_move(const GameState& state, int start_row, int start_col, int end_row,
                              int end_col) {
        return state.is_legal_move(start_row, start_col, end_row, end_col);
    }

    static BoardModel& board(GameState& state) { return state.board_; }

    static PieceState piece_state(const GameState& state, Piece::Id id) {
        return state.board_.get_piece(id).state;
    }
};

inline BoardModel make_board(std::initializer_list<std::initializer_list<const char*>> rows) {
    return BoardModel::from_token_grid(rows);
}

using BoardLayout = std::vector<std::vector<std::string>>;

inline BoardLayout capture_layout(const GameState& state) {
    BoardLayout layout(state.rows(), std::vector<std::string>(state.cols()));
    for (std::size_t row = 0; row < state.rows(); ++row) {
        for (std::size_t col = 0; col < state.cols(); ++col) {
            layout[row][col] = state.token_at(row, col);
        }
    }
    return layout;
}

inline bool layout_matches(const GameState& state, const BoardLayout& layout) {
    if (state.rows() != layout.size()) {
        return false;
    }
    for (std::size_t row = 0; row < state.rows(); ++row) {
        if (state.cols() != layout[row].size()) {
            return false;
        }
        for (std::size_t col = 0; col < state.cols(); ++col) {
            if (state.token_at(row, col) != layout[row][col]) {
                return false;
            }
        }
    }
    return true;
}

inline PendingMove make_pending_move(const BoardModel& board, std::size_t row, std::size_t col,
                                     std::pair<std::size_t, std::size_t> end_pos,
                                     std::int64_t arrival_time) {
    const Piece* piece = board.piece_at(row, col);
    assert(piece != nullptr);
    return PendingMove{piece->id, piece->color, piece->kind, {row, col}, end_pos, 0, arrival_time};
}

inline JumpState make_jump_state(const BoardModel& board, std::size_t row, std::size_t col,
                                 std::int64_t arrival_time) {
    const Piece* piece = board.piece_at(row, col);
    assert(piece != nullptr);
    return JumpState{piece->id, piece->color, piece->kind, {row, col}, 0, arrival_time};
}

inline ArrivingPieceInfo make_arriving_info(const BoardModel& board, std::size_t start_row,
                                            std::size_t start_col,
                                            std::pair<std::size_t, std::size_t> end_pos) {
    const Piece* piece = board.piece_at(start_row, start_col);
    assert(piece != nullptr);
    return ArrivingPieceInfo{piece->id, {start_row, start_col}, end_pos};
}

inline Piece make_piece(PieceColor color, PieceKind kind) {
    PieceFactory factory;
    return factory.create(color, kind, {0, 0});
}

}  // namespace kfc::test
