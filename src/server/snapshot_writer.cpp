#include "snapshot_writer.h"

#include "model/piece_token.h"

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
    return out.str();
}

}  // namespace kfc
