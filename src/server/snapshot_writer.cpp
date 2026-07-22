#include "snapshot_writer.h"

#include "model/piece_token.h"

#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>

namespace kfc {

namespace {

void write_cell_token(std::ostream& out, const CellView& cell) {
    if (cell.piece.has_value()) {
        out << to_token(cell.piece->color, cell.piece->kind);
    } else {
        out << '.';
    }
}

void write_progress(std::ostream& out, float progress) {
    out << std::fixed << std::setprecision(6) << progress;
}

const char* rest_kind_token(RestKind kind) {
    switch (kind) {
        case RestKind::Short:
            return "short";
        case RestKind::Long:
            return "long";
    }
    return "short";
}

void write_move_snapshot(std::ostream& out, const ActiveMoveSnapshot& move) {
    out << "move piece_id " << move.piece_id << " piece "
        << to_token(move.color, move.kind) << " from " << move.from_row << ' ' << move.from_col
        << " to " << move.to_row << ' ' << move.to_col << " progress ";
    write_progress(out, move.progress);
    out << '\n';
}

void write_jump_snapshot(std::ostream& out, const ActiveJumpSnapshot& jump) {
    out << "jump piece_id " << jump.piece_id << " piece "
        << to_token(jump.color, jump.kind) << " row " << jump.row << ' ' << jump.col
        << " progress ";
    write_progress(out, jump.progress);
    out << '\n';
}

void write_rest_snapshot(std::ostream& out, const ActiveRestSnapshot& rest) {
    out << "rest piece_id " << rest.piece_id << " row " << rest.row << ' ' << rest.col
        << " kind " << rest_kind_token(rest.kind) << " progress ";
    write_progress(out, rest.progress);
    out << '\n';
}

}  // namespace

std::string write_snapshot(const BoardViewModel& view) {
    std::ostringstream out;
    out << "snapshot\n";
    out << "clock_ms " << view.clock_ms << '\n';
    out << "game_over " << (view.game_over ? "true" : "false") << '\n';
    out << "height " << view.height << '\n';
    out << "width " << view.width << '\n';
    if (view.selection.has_value()) {
        out << "selection " << view.selection->first << ' ' << view.selection->second << '\n';
    }
    out << "cells\n";
    for (std::size_t row = 0; row < view.height; ++row) {
        for (std::size_t col = 0; col < view.width; ++col) {
            if (col > 0) {
                out << ' ';
            }
            write_cell_token(out, board_view_cell_at(view, row, col));
        }
        if (row + 1 < view.height) {
            out << '\n';
        }
    }
    out << "\nanimations\nmoves\n";
    for (const ActiveMoveSnapshot& move : view.animations.moves) {
        write_move_snapshot(out, move);
    }
    out << "jumps\n";
    for (const ActiveJumpSnapshot& jump : view.animations.jumps) {
        write_jump_snapshot(out, jump);
    }
    out << "rests\n";
    for (const ActiveRestSnapshot& rest : view.animations.rests) {
        write_rest_snapshot(out, rest);
    }
    return out.str();
}

}  // namespace kfc
