#include "logic/game_state.h"
#include "model/game_config.h"
#include "test_helpers.h"
#include "ui/headless_renderer.h"
#include "ui/ui_controller.h"

#include <doctest/doctest.h>
#include <memory>

TEST_CASE("UiControllerTest - InitializesHeadlessRenderer") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();

    kfc::UiController controller(state, std::move(renderer));

    CHECK(renderer_ptr->initialized());
    CHECK_EQ(renderer_ptr->rows(), 1u);
    CHECK_EQ(renderer_ptr->cols(), 3u);
    CHECK_EQ(renderer_ptr->cell_pixel_size(), kfc::kCellPixelSize);
    CHECK_EQ(renderer_ptr->input_sink(), &controller);
}

TEST_CASE("UiControllerTest - FrameAdvancesClockAndPresentsView") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();
    kfc::UiController controller(state, std::move(renderer));

    const kfc::UiFrameResult result = controller.frame(16);

    CHECK(result.should_continue);
    CHECK_EQ(state.clock_ms(), 16);
    CHECK_EQ(renderer_ptr->present_count(), 1u);
    CHECK_EQ(renderer_ptr->last_view().clock_ms, 16);
    CHECK_EQ(renderer_ptr->last_view().cells.size(), 3u);
}

TEST_CASE("UiControllerTest - PixelClickRoutesThroughController") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::UiController controller(state, std::make_unique<kfc::HeadlessRenderer>());

    controller.on_pixel_click(50, 50);

    CHECK(state.has_selection());
    std::size_t row = 0;
    std::size_t col = 0;
    REQUIRE(state.selection(row, col));
    CHECK_EQ(row, 0u);
    CHECK_EQ(col, 0u);
}

TEST_CASE("UiControllerTest - FrameStopsWhenRendererRequestsQuit") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();
    renderer_ptr->set_should_continue(false);
    kfc::UiController controller(state, std::move(renderer));

    const kfc::UiFrameResult result = controller.frame(16);

    CHECK_FALSE(result.should_continue);
}

TEST_CASE("UiControllerTest - ShutdownDelegatesToRenderer") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();
    kfc::UiController controller(state, std::move(renderer));

    controller.shutdown();

    CHECK(renderer_ptr->shutdown_called());
    CHECK_FALSE(renderer_ptr->initialized());
}
