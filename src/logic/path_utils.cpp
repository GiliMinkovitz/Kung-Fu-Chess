#include "path_utils.h"

#include <cstdlib>

namespace kfc {

namespace {

[[nodiscard]] bool is_straight_or_diagonal_path(int dr, int dc) noexcept {
    if (dr == 0 || dc == 0) {
        return true;
    }
    return std::abs(dr) == std::abs(dc);
}

}  // namespace

void for_each_cell_on_path(int start_row, int start_col, int end_row, int end_col,
                           const std::function<void(int row, int col)>& visitor) {
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    const int step_r = (dr == 0) ? 0 : (dr > 0 ? 1 : -1);
    const int step_c = (dc == 0) ? 0 : (dc > 0 ? 1 : -1);

    int row = start_row + step_r;
    int col = start_col + step_c;
    while (row != end_row || col != end_col) {
        visitor(row, col);
        row += step_r;
        col += step_c;
    }
}

bool paths_share_cell(const Cell& a_start, const Cell& a_end, const Cell& b_start,
                      const Cell& b_end) {
    const int a_dr = static_cast<int>(a_end.first) - static_cast<int>(a_start.first);
    const int a_dc = static_cast<int>(a_end.second) - static_cast<int>(a_start.second);
    const int b_dr = static_cast<int>(b_end.first) - static_cast<int>(b_start.first);
    const int b_dc = static_cast<int>(b_end.second) - static_cast<int>(b_start.second);

    if (!is_straight_or_diagonal_path(a_dr, a_dc) || !is_straight_or_diagonal_path(b_dr, b_dc)) {
        return a_start == b_start || a_start == b_end || a_end == b_start || a_end == b_end;
    }

    const int a_step_r = (a_dr == 0) ? 0 : (a_dr > 0 ? 1 : -1);
    const int a_step_c = (a_dc == 0) ? 0 : (a_dc > 0 ? 1 : -1);
    const int b_step_r = (b_dr == 0) ? 0 : (b_dr > 0 ? 1 : -1);
    const int b_step_c = (b_dc == 0) ? 0 : (b_dc > 0 ? 1 : -1);

    int a_row = static_cast<int>(a_start.first);
    int a_col = static_cast<int>(a_start.second);
    while (true) {
        int b_row = static_cast<int>(b_start.first);
        int b_col = static_cast<int>(b_start.second);
        while (true) {
            if (a_row == b_row && a_col == b_col) {
                return true;
            }
            if (b_row == static_cast<int>(b_end.first) &&
                b_col == static_cast<int>(b_end.second)) {
                break;
            }
            b_row += b_step_r;
            b_col += b_step_c;
        }

        if (a_row == static_cast<int>(a_end.first) && a_col == static_cast<int>(a_end.second)) {
            break;
        }
        a_row += a_step_r;
        a_col += a_step_c;
    }

    return false;
}

}  // namespace kfc
