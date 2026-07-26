#include "server/rating_service.h"

#include <doctest/doctest.h>

TEST_CASE("RatingServiceTest - EqualRatingsChangeSymmetrically") {
    kfc::RatingService service;
    const kfc::RatingChange change = service.calculate(1000, 1000);

    CHECK_EQ(change.winner_new_rating, 1013);
    CHECK_EQ(change.loser_new_rating, 987);
    CHECK_EQ(change.winner_new_rating - 1000, 1000 - change.loser_new_rating);
}

TEST_CASE("RatingServiceTest - HigherRatedPlayerWinsGainsLess") {
    kfc::RatingService service;
    const kfc::RatingChange change = service.calculate(1200, 1000);

    const int winner_gain = change.winner_new_rating - 1200;
    const int loser_loss = 1000 - change.loser_new_rating;
    CHECK(winner_gain < service.k_factor() / 2);
    CHECK_EQ(winner_gain, loser_loss);
}

TEST_CASE("RatingServiceTest - LowerRatedPlayerWinsGainsMore") {
    kfc::RatingService service;
    const kfc::RatingChange change = service.calculate(1000, 1200);

    const int winner_gain = change.winner_new_rating - 1000;
    const int loser_loss = 1200 - change.loser_new_rating;
    CHECK(winner_gain > service.k_factor() / 2);
    CHECK_EQ(winner_gain, loser_loss);
}

TEST_CASE("RatingServiceTest - RatingChangesAreSymmetric") {
    kfc::RatingService service;
    const kfc::RatingChange change = service.calculate(1050, 980);

    CHECK_EQ(change.winner_new_rating - 1050, 980 - change.loser_new_rating);
}

TEST_CASE("RatingServiceTest - ClampsLoserRatingAtZero") {
    kfc::RatingService service;
    const kfc::RatingChange change = service.calculate(50, 10);

    CHECK_EQ(change.loser_new_rating, 0);
    CHECK(change.winner_new_rating > 50);
}

TEST_CASE("RatingServiceTest - ConfigurableKFactor") {
    kfc::RatingService service{32};
    const kfc::RatingChange change = service.calculate(1000, 1000);

    CHECK_EQ(change.winner_new_rating, 1016);
    CHECK_EQ(change.loser_new_rating, 984);
}
