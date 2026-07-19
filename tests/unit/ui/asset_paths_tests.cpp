#include "ui/assets/asset_paths.h"
#include "ui/assets/theme_config.h"

#include <doctest/doctest.h>

TEST_CASE("AssetPathsTest - ResolvesThemePaths") {
    const kfc::ThemeConfig theme{
        std::filesystem::path{"assets/board.png"},
        std::filesystem::path{"assets/pieces"},
    };
    const kfc::AssetPaths paths(theme);

    CHECK(paths.board() == theme.board_image);
    CHECK(paths.piece_directory(kfc::PieceColor::White, kfc::PieceKind::King) ==
          theme.pieces_directory / "wK" / "states");
    CHECK(paths.piece_directory(kfc::PieceColor::Black, kfc::PieceKind::Pawn) ==
          theme.pieces_directory / "bP" / "states");
    CHECK(paths.piece_sprite(kfc::PieceColor::White, kfc::PieceKind::Rook, "move", 2) ==
          theme.pieces_directory / "wR" / "states" / "move" / "sprites" / "2.png");
}
