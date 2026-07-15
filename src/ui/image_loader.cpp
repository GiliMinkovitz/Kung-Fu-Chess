#include "image_loader.h"

namespace kfc {

ImageLoader::ImageLoader(const AssetPaths& paths) : paths_(paths) {}

Img ImageLoader::load_board() const {
    Img img;
    img.read(paths_.board().string());
    return img;
}

}  // namespace kfc
