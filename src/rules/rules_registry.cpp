#include "rules_registry.h"

#include "piece_rules/bishop_rules.h"
#include "piece_rules/king_rules.h"
#include "piece_rules/knight_rules.h"
#include "piece_rules/pawn_rules.h"
#include "piece_rules/queen_rules.h"
#include "piece_rules/rook_rules.h"

#include <array>
#include <cstddef>

namespace kfc::piece_rules {

namespace {

constexpr std::size_t kPieceKindCount = static_cast<std::size_t>(PieceKind::Count);

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
