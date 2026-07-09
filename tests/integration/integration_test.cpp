#include "adapters/board_writer.h"
#include "adapters/command_processor.h"
#include "core/board_model.h"
#include "logic/game_state.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>

namespace {

std::string capture_board(kfc::GameState& state) {
    std::ostringstream out;
    state.write_board(out, kfc::write_board);
    return out.str();
}

}  // namespace

TEST_CASE("IntegrationTest - CommandSequenceWriteBoardGroundTruth") {
    std::ostringstream sink;

    SUBCASE("KingMoveSequence") {
        kfc::GameState state(kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}}));
        kfc::CommandProcessor processor(state);

        CHECK_EQ(capture_board(state), "wK . bK\n. . .");

        processor.execute("click 50 50", sink);
        processor.execute("click 150 150", sink);
        CHECK_EQ(capture_board(state), "wK . bK\n. . .");

        processor.execute("wait 1000", sink);
        CHECK_EQ(capture_board(state), ". . bK\n. wK .");
    }

    SUBCASE("RookCaptureSequence") {
        kfc::GameState state(kfc::test::make_board({{"wR", ".", "bK"}}));
        kfc::CommandProcessor processor(state);

        CHECK_EQ(capture_board(state), "wR . bK");

        processor.execute("click 50 50", sink);
        processor.execute("click 250 50", sink);
        CHECK_EQ(capture_board(state), "wR . bK");

        processor.execute("wait 2000", sink);
        CHECK_EQ(capture_board(state), ". . wR");
    }
}
