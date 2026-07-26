#include "server/rating_service.h"

#include <cmath>

namespace kfc {

RatingService::RatingService(int k_factor) : k_factor_(k_factor) {}

int RatingService::k_factor() const noexcept {
    return k_factor_;
}

int RatingService::clamp_rating(int rating) noexcept {
    if (rating < kMinRating) {
        return kMinRating;
    }
    return rating;
}

double RatingService::expected_score(int player_rating, int opponent_rating) noexcept {
    return 1.0 / (1.0 + std::pow(10.0, (opponent_rating - player_rating) / 400.0));
}

RatingChange RatingService::calculate(int winner_rating, int loser_rating) const {
    const double winner_expected = expected_score(winner_rating, loser_rating);
    const double loser_expected = expected_score(loser_rating, winner_rating);

    const int winner_new =
        clamp_rating(winner_rating + static_cast<int>(std::lround(k_factor_ * (1.0 - winner_expected))));
    const int loser_new =
        clamp_rating(loser_rating + static_cast<int>(std::lround(k_factor_ * (0.0 - loser_expected))));

    return RatingChange{winner_new, loser_new};
}

}  // namespace kfc
