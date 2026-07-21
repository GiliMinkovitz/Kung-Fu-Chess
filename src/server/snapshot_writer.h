#pragma once

#include "ui/view/board_view_model.h"

#include <string>

namespace kfc {

[[nodiscard]] std::string write_snapshot(const BoardViewModel& view);

}  // namespace kfc
