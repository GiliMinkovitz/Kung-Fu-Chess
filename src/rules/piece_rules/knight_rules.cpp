#include "knight_rules.h"

#include <cstdlib>

namespace kfc::piece_rules {

bool is_knight_move(int dr, int dc) noexcept {
    const int adr = std::abs(dr);
    const int adc = std::abs(dc);
    return (adr == 2 && adc == 1) || (adr == 1 && adc == 2);
}

}  // namespace kfc::piece_rules
