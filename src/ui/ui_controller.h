#pragma once

#include "i_ui_input_sink.h"
#include "i_ui_renderer.h"
#include "../io/command_processor.h"
#include "../logic/game_state.h"

#include <cstdint>
#include <memory>

namespace kfc {

class UiController final : public IUiInputSink {
public:
    UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer);

    UiController(const UiController&) = delete;
    UiController& operator=(const UiController&) = delete;

    [[nodiscard]] UiFrameResult frame(std::int64_t delta_ms);
    void on_pixel_click(int x, int y) override;
    void on_pixel_jump(int x, int y) override;
    void shutdown();

private:
    GameState& state_;
    CommandProcessor processor_;
    std::unique_ptr<IUiRenderer> renderer_;
    int cell_pixel_size_ = 0;
};

}  // namespace kfc
