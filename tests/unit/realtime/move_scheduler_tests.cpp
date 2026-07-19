#include "realtime/move_scheduler.h"
#include "realtime/render_snapshot.h"

#include <doctest/doctest.h>

TEST_CASE("MoveSchedulerTest - AnimationsAtIncludesActiveMoveWithPieceIdentity") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_move(
        {1, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(50);

    REQUIRE_EQ(snapshot.moves.size(), 1u);
    const kfc::ActiveMoveSnapshot& move = snapshot.moves.front();
    CHECK_EQ(move.piece_id, 1u);
    CHECK(move.kind == kfc::PieceKind::Rook);
    CHECK(move.color == kfc::PieceColor::White);
    CHECK_EQ(move.from_row, 0u);
    CHECK_EQ(move.from_col, 0u);
    CHECK_EQ(move.to_row, 0u);
    CHECK_EQ(move.to_col, 2u);
    CHECK(move.progress == doctest::Approx(0.5f));
}

TEST_CASE("MoveSchedulerTest - AnimationsAtSkipsExpiredMoves") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_move({1, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, {0, 2}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);

    CHECK(snapshot.moves.empty());
    CHECK(snapshot.jumps.empty());
}

TEST_CASE("MoveSchedulerTest - AnimationsAtIncludesActiveJumpWithPieceIdentity") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_jump(
        {1, kfc::PieceColor::White, kfc::PieceKind::King, {0, 0}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(50);

    REQUIRE_EQ(snapshot.jumps.size(), 1u);
    const kfc::ActiveJumpSnapshot& jump = snapshot.jumps.front();
    CHECK_EQ(jump.piece_id, 1u);
    CHECK(jump.kind == kfc::PieceKind::King);
    CHECK(jump.color == kfc::PieceColor::White);
    CHECK_EQ(jump.row, 0u);
    CHECK_EQ(jump.col, 0u);
    CHECK(jump.progress == doctest::Approx(0.5f));
}

TEST_CASE("MoveSchedulerTest - AnimationsAtSkipsExpiredJumps") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_jump({1, kfc::PieceColor::White, kfc::PieceKind::Rook, {0, 0}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);

    CHECK(snapshot.moves.empty());
    CHECK(snapshot.jumps.empty());
    CHECK(snapshot.rests.empty());
}

TEST_CASE("MoveSchedulerTest - AnimationsAtIncludesActiveRest") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_rest(1, kfc::RestKind::Long, 100, 600, 2, 3);

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(350);

    REQUIRE_EQ(snapshot.rests.size(), 1u);
    const kfc::ActiveRestSnapshot& rest = snapshot.rests.front();
    CHECK_EQ(rest.piece_id, 1u);
    CHECK_EQ(rest.row, 2u);
    CHECK_EQ(rest.col, 3u);
    CHECK(rest.kind == kfc::RestKind::Long);
    CHECK(rest.progress == doctest::Approx(0.5f));
}

TEST_CASE("MoveSchedulerTest - AnimationsAtSkipsExpiredRests") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_rest(1, kfc::RestKind::Short, 0, 100, 0, 0);

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);

    CHECK(snapshot.rests.empty());
}

TEST_CASE("MoveSchedulerTest - RestBlockingAndSnapshotAlignAtStart") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_rest(1, kfc::RestKind::Long, 100, 600, 2, 3);

    CHECK(scheduler.is_piece_resting(100, 1));
    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);
    REQUIRE_EQ(snapshot.rests.size(), 1u);
    CHECK(snapshot.rests.front().progress == doctest::Approx(0.0f));
}

TEST_CASE("MoveSchedulerTest - RestBlockingAndSnapshotAlignAtEnd") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_rest(1, kfc::RestKind::Short, 0, 100, 0, 0);

    CHECK_FALSE(scheduler.is_piece_resting(100, 1));
    CHECK(scheduler.animations_at(100).rests.empty());
}

TEST_CASE("MoveSchedulerTest - RestBlockingAndSnapshotAlignBeforeEnd") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_rest(1, kfc::RestKind::Long, 0, 100, 0, 0);

    CHECK(scheduler.is_piece_resting(99, 1));
    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(99);
    REQUIRE_EQ(snapshot.rests.size(), 1u);
    CHECK(snapshot.rests.front().progress == doctest::Approx(0.99f));
}

TEST_CASE("RenderSnapshotTest - ComputeAnimationProgressAtOrAfterArrival") {
    CHECK(kfc::compute_animation_progress(100, 0, 100) == doctest::Approx(1.0f));
    CHECK(kfc::compute_animation_progress(150, 0, 100) == doctest::Approx(1.0f));
}

TEST_CASE("RenderSnapshotTest - ComputeAnimationProgressAtOrBeforeStart") {
    CHECK(kfc::compute_animation_progress(0, 0, 100) == doctest::Approx(0.0f));
    CHECK(kfc::compute_animation_progress(-10, 0, 100) == doctest::Approx(0.0f));
}
