#pragma once

#include <cstddef>
#include <functional>
#include <utility>

namespace kfc {

using Cell = std::pair<std::size_t, std::size_t>;

void for_each_cell_on_path(int start_row, int start_col, int end_row, int end_col,
                           const std::function<void(int row, int col)>& visitor);

[[nodiscard]] bool paths_share_cell(const Cell& a_start, const Cell& a_end, const Cell& b_start,
                                    const Cell& b_end);

}  // namespace kfc
