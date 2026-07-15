#pragma once

#include "asset_paths.h"

#include <img.hpp>

#include <utility>

namespace kfc {

class ImageLoader {
public:
    explicit ImageLoader(const AssetPaths& paths);

    [[nodiscard]] Img load_board(std::pair<int, int> size) const;

private:
    AssetPaths paths_;
};

}  // namespace kfc
