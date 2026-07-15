#pragma once

#include <filesystem>

namespace kfc {

struct ThemeConfig {
    std::filesystem::path board_image;
    std::filesystem::path pieces_directory;
};

inline const ThemeConfig kDefaultCtd26ThemeConfig{
    std::filesystem::path{"third_party/CTD26/board_classic.png"},
    std::filesystem::path{"third_party/CTD26/pieces"},
};

}  // namespace kfc
