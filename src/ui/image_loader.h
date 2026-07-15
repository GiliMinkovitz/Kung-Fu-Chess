#pragma once

#include "asset_paths.h"

#include <img.hpp>

namespace kfc {

class ImageLoader {
public:
    explicit ImageLoader(const AssetPaths& paths);

    [[nodiscard]] Img load_board() const;

private:
    AssetPaths paths_;
};

}  // namespace kfc
