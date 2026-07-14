#pragma once

namespace kfc {

struct UiColor {
    int b = 0;
    int g = 0;
    int r = 0;
};

struct UiTheme {
    UiColor frame_background{30, 30, 30};
    UiColor light_cell{220, 220, 220};
    UiColor dark_cell{100, 100, 100};
    UiColor selection_border{255, 220, 0};
    UiColor jump_border{255, 180, 0};
    UiColor white_token{20, 20, 20};
    UiColor black_token{240, 240, 240};
    UiColor game_over_text{255, 0, 0};

    double token_font_scale = 1.2;
    int token_thickness = 2;
    int selection_border_thickness = 3;
    int jump_border_thickness = 2;
    float jump_lift_ratio = 0.25f;

    double game_over_font_scale = 1.0;
    int game_over_thickness = 2;
    int game_over_x = 20;
    int game_over_y = 40;
};

inline constexpr UiTheme kDefaultUiTheme{};

}  // namespace kfc
