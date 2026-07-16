#pragma once

#include "ui/assets/asset_paths.h"

#include "model/piece.h"

#include <img.hpp>

#include <string_view>
#include <utility>

namespace kfc {

class ImageLoader {
public:
    explicit ImageLoader(const AssetPaths& paths);

    [[nodiscard]] Img load_board(std::pair<int, int> size) const;
    [[nodiscard]] Img load_piece_sprite(PieceColor color, PieceKind kind, std::string_view state,
                                        int frame, std::pair<int, int> size) const;

private:
    AssetPaths paths_;
};

}  // namespace kfc
