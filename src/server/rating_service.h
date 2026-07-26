#pragma once

namespace kfc {

struct RatingChange {
    int winner_new_rating;
    int loser_new_rating;
};

class RatingService {
public:
    static constexpr int kDefaultKFactor = 25;
    static constexpr int kMinRating = 0;

    explicit RatingService(int k_factor = kDefaultKFactor);

    [[nodiscard]] RatingChange calculate(int winner_rating, int loser_rating) const;
    [[nodiscard]] int k_factor() const noexcept;

private:
    [[nodiscard]] static int clamp_rating(int rating) noexcept;
    [[nodiscard]] static double expected_score(int player_rating, int opponent_rating) noexcept;

    int k_factor_;
};

}  // namespace kfc
