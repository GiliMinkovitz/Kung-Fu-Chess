#include "engine/game_engine.h"
#include "io/game_input_handler.h"
#include "test_helpers.h"

#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <vector>

TEST_CASE("GameEngineTest - ExecutePrintBoardDelegatesToGameInputHandler") {
    const kfc::BoardModel board = kfc::test::make_board({{"wK", ".", "bK"}});
    kfc::GameEngine engine(board);
    kfc::GameState reference_state(board);
    kfc::GameInputHandler reference_processor(reference_state);

    std::ostringstream engine_out;
    std::ostringstream processor_out;

    engine.execute("print board", engine_out);
    reference_processor.execute("print board", processor_out);

    CHECK_EQ(engine_out.str(), processor_out.str());
    CHECK_EQ(engine_out.str(), "wK . bK\n");
}

TEST_CASE("GameEngineTest - ExecuteNonPrintCommandWritesNothingToStream") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::ostringstream out;

    engine.execute("click 50 50", out);
    CHECK(out.str().empty());
    CHECK(engine.state().has_selection());
}

TEST_CASE("GameEngineTest - ExecuteAllRunsCommandsInOrder") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}, {".", ".", "."}}));
    std::ostringstream out;

    engine.execute_all({"click 50 50", "click 150 150", "wait 1000", "print board"}, out);
    CHECK_EQ(out.str(), ". . bK\n. wK .\n");
}

TEST_CASE("GameEngineTest - ExecuteAllEmptyCommandList") {
    kfc::GameEngine engine(kfc::test::make_board({{"wK", ".", "bK"}}));
    std::ostringstream out;

    engine.execute_all({}, out);
    CHECK(out.str().empty());
}
