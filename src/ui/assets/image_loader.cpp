#include "ui/assets/image_loader.h"

namespace kfc {

ImageLoader::ImageLoader(const AssetPaths& paths) : paths_(paths) {}

Img ImageLoader::load_board(const std::pair<int, int> size) const {
    Img img;
    img.read(paths_.board().string(), size, false, cv::INTER_LINEAR);
    return img;
}

Img ImageLoader::load_piece_sprite(const PieceColor color, const PieceKind kind,
                                   const std::string_view state, const int frame,
                                   const std::pair<int, int> size) const {
    Img img;
    img.read(paths_.piece_sprite(color, kind, std::string(state), frame).string(), size, false,
             cv::INTER_LINEAR);
    return img;
}

}  // namespace kfc
