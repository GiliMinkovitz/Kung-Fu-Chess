#pragma once

#include "ui/rendering/i_ui_input_sink.h"
#include "ui/rendering/i_ui_renderer.h"
#include "io/game_input_handler.h"
#include "logic/game_state.h"

#include <cstdint>
#include <memory>

namespace kfc {

// GUI application loop: advances the realtime clock each frame, builds a view model
// for rendering, and routes mouse input through GameInputHandler. Does not draw
// pixels or implement rules; collaborates with GameState, GameInputHandler, and
// IUiRenderer.
class UiController final : public IUiInputSink {
public:
    UiController(GameState& state, std::unique_ptr<IUiRenderer> renderer);

    UiController(const UiController&) = delete;
    UiController& operator=(const UiController&) = delete;

    // Unlike CLI "wait" commands, the GUI drives time continuously via delta_ms.
    [[nodiscard]] UiFrameResult frame(std::int64_t delta_ms);
    void on_pixel_click(int x, int y) override;
    void on_pixel_jump(int x, int y) override;
    void shutdown();

private:
    void sync_input_layout();

    GameState& state_;
    GameInputHandler processor_;
    std::unique_ptr<IUiRenderer> renderer_;
};

}  // namespace kfc
