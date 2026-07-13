#include "rules_registry.h"

#include "bishop_rules.h"
#include "king_rules.h"
#include "knight_rules.h"
#include "pawn_rules.h"
#include "queen_rules.h"
#include "rook_rules.h"

#include <array>
#include <cstddef>

namespace kfc::piece_rules {

namespace {

[[nodiscard]] bool validate_knight_move(const BoardModel& board, int start_row, int start_col,
                                        int end_row, int end_col) {
    (void)board;
    const int dr = end_row - start_row;
    const int dc = end_col - start_col;
    return is_knight_move(dr, dc);
}

constexpr std::size_t kPieceKindCount = 6;

const std::array<PieceRuleEntry, kPieceKindCount> kRules = {{
    {is_king_move, true},           // PieceKind::King
    {is_queen_move, true},          // PieceKind::Queen
    {is_rook_move, true},           // PieceKind::Rook
    {is_bishop_move, true},         // PieceKind::Bishop
    {validate_knight_move, true},   // PieceKind::Knight
    {is_pawn_move, false},          // PieceKind::Pawn
}};

}  // namespace

PieceRuleEntry get_rule_entry(PieceKind kind) noexcept {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= kRules.size()) {
        return {};
    }
    return kRules[index];
}

}  // namespace kfc::piece_rules
