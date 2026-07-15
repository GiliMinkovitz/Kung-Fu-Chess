#pragma once

namespace kfc {

struct UiColor {
    int b = 0;
    int g = 0;
    int r = 0;
};

struct UiTheme {
    UiColor frame_background{30, 30, 30};
    UiColor selection_border{255, 220, 0};
    UiColor jump_border{255, 180, 0};
    UiColor white_token{20, 20, 20};
    UiColor black_token{240, 240, 240};
    UiColor game_over_text{255, 0, 0};

    float jump_lift_ratio = 0.25f;
};

inline constexpr UiTheme kDefaultUiTheme{};

}  // namespace kfc
