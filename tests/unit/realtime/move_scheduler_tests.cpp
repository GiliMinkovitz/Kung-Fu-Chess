#include "realtime/move_scheduler.h"
#include "realtime/render_snapshot.h"

#include <doctest/doctest.h>

TEST_CASE("MoveSchedulerTest - AnimationsAtSkipsExpiredMoves") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_move({1, kfc::PieceColor::White, {0, 0}, {0, 2}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);

    CHECK(snapshot.moves.empty());
    CHECK(snapshot.jumps.empty());
}

TEST_CASE("MoveSchedulerTest - AnimationsAtSkipsExpiredJumps") {
    kfc::MoveScheduler scheduler;
    scheduler.schedule_jump({1, kfc::PieceColor::White, {0, 0}, 0, 100});

    const kfc::AnimationSnapshot snapshot = scheduler.animations_at(100);

    CHECK(snapshot.moves.empty());
    CHECK(snapshot.jumps.empty());
}

TEST_CASE("RenderSnapshotTest - ComputeAnimationProgressAtOrAfterArrival") {
    CHECK(kfc::compute_animation_progress(100, 0, 100) == doctest::Approx(1.0f));
    CHECK(kfc::compute_animation_progress(150, 0, 100) == doctest::Approx(1.0f));
}

TEST_CASE("RenderSnapshotTest - ComputeAnimationProgressAtOrBeforeStart") {
    CHECK(kfc::compute_animation_progress(0, 0, 100) == doctest::Approx(0.0f));
    CHECK(kfc::compute_animation_progress(-10, 0, 100) == doctest::Approx(0.0f));
}
