#pragma once

#include "board_view_model.h"
#include "i_ui_input_sink.h"

#include <cstddef>

namespace kfc {

struct UiFrameResult {
    bool should_continue = true;
};

class IUiRenderer {
public:
    virtual ~IUiRenderer() = default;
    virtual void init(std::size_t rows, std::size_t cols, int cell_pixel_size) = 0;
    virtual void attach_input_sink(IUiInputSink* sink) = 0;
    virtual UiFrameResult present(const BoardViewModel& view) = 0;
    virtual void shutdown() = 0;
};

}  // namespace kfc
