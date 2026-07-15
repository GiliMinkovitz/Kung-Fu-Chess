#pragma once

#include <cstddef>

namespace kfc {

struct UiWindowDimensions {
    int width = 0;
    int height = 0;
};

// Bootstrap-only default window size that preserves the legacy visual output.
// Actual cell size comes from BoardLayoutCalculator after init.
[[nodiscard]] inline UiWindowDimensions default_initial_window_size(std::size_t rows,
                                                                    std::size_t cols) noexcept {
    constexpr int kLegacyInitialPixelsPerBoardCell = 100;
    return {static_cast<int>(cols) * kLegacyInitialPixelsPerBoardCell,
            static_cast<int>(rows) * kLegacyInitialPixelsPerBoardCell};
}

}  // namespace kfc
