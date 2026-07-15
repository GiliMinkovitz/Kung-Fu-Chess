#include "asset_paths.h"

#include "../model/piece_token.h"

#include <string>

namespace kfc {

AssetPaths::AssetPaths(const ThemeConfig& theme) : theme_(theme) {}

std::filesystem::path AssetPaths::board() const {
    return theme_.board_image;
}

std::filesystem::path AssetPaths::piece_directory(PieceColor color, PieceKind kind) const {
    const std::string token{color_to_char(color), kind_to_char(kind)};
    return theme_.pieces_directory / token / "states";
}

}  // namespace kfc
