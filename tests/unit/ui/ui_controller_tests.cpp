#include "logic/game_state.h"
#include "test_helpers.h"
#include "ui/rendering/headless_renderer.h"
#include "ui/controller/ui_controller.h"
#include "ui/layout/ui_window_config.h"
#include "ui/view/board_view_model.h"

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
    const kfc::UiWindowDimensions window = kfc::default_initial_window_size(1u, 3u);
    CHECK_EQ(renderer_ptr->window_width(), window.width);
    CHECK_EQ(renderer_ptr->window_height(), window.height);
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

TEST_CASE("UiControllerTest - PixelJumpRoutesThroughController") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    kfc::UiController controller(state, std::make_unique<kfc::HeadlessRenderer>());

    controller.on_pixel_jump(50, 50);

    CHECK(state.is_piece_jumping(0, 0));
    CHECK_FALSE(state.has_selection());
}

TEST_CASE("UiControllerTest - PresentRendersViewWithoutMutatingGameState") {
    kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}}));
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();
    kfc::UiController controller(state, std::move(renderer));

    kfc::BoardViewModel view;
    view.height = 1;
    view.width = 3;
    view.clock_ms = 999;
    view.cells = {
        kfc::CellView{kfc::PieceView{kfc::PieceKind::King, kfc::PieceColor::White}},
        kfc::CellView{},
        kfc::CellView{kfc::PieceView{kfc::PieceKind::King, kfc::PieceColor::Black}},
    };

    const std::int64_t clock_before = state.clock_ms();
    const kfc::UiFrameResult result = controller.present(view);

    CHECK(result.should_continue);
    CHECK_EQ(state.clock_ms(), clock_before);
    CHECK_EQ(renderer_ptr->present_count(), 1u);
    CHECK_EQ(renderer_ptr->last_view().clock_ms, 999);
    CHECK_EQ(renderer_ptr->last_view().cells.size(), 3u);
}

TEST_CASE("UiControllerTest - NetworkConstructorInitializesWithoutGameState") {
    auto renderer = std::make_unique<kfc::HeadlessRenderer>();
    kfc::HeadlessRenderer* renderer_ptr = renderer.get();

    kfc::UiController controller(8, 8, std::move(renderer));

    CHECK(renderer_ptr->initialized());
    CHECK_EQ(renderer_ptr->rows(), 8u);
    CHECK_EQ(renderer_ptr->cols(), 8u);
    const kfc::UiWindowDimensions window = kfc::default_initial_window_size(8u, 8u);
    CHECK_EQ(renderer_ptr->window_width(), window.width);
    CHECK_EQ(renderer_ptr->window_height(), window.height);
    CHECK_EQ(renderer_ptr->input_sink(), &controller);
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
