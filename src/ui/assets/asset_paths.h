#pragma once

#include "ui/assets/theme_config.h"

#include "model/piece.h"

#include <filesystem>
#include <string>

namespace kfc {

class AssetPaths {
public:
    explicit AssetPaths(const ThemeConfig& theme);

    [[nodiscard]] std::filesystem::path board() const;
    [[nodiscard]] std::filesystem::path piece_directory(PieceColor color, PieceKind kind) const;
    [[nodiscard]] std::filesystem::path piece_sprite(PieceColor color, PieceKind kind,
                                                     const std::string& state, int frame) const;

private:
    ThemeConfig theme_;
};

}  // namespace kfc
