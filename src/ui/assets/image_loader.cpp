#include "ui/assets/image_loader.h"

namespace kfc {

ImageLoader::ImageLoader(const AssetPaths& paths) : paths_(paths) {}

Img ImageLoader::load_board(const std::pair<int, int> size) const {
    Img img;
    img.read(paths_.board().string(), size, false, cv::INTER_LINEAR);
    return img;
}

}  // namespace kfc
