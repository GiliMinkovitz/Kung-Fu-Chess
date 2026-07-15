#pragma once

#include <filesystem>

namespace kfc {

struct ThemeConfig {
    std::filesystem::path board_image;
    std::filesystem::path pieces_directory;
};

}  // namespace kfc
