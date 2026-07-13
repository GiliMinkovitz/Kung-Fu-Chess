#pragma once

#include "piece_rules/piece_rules_interface.h"

#include "../model/piece.h"

namespace kfc::piece_rules {

[[nodiscard]] PieceRuleEntry get_rule_entry(PieceKind kind) noexcept;

}  // namespace kfc::piece_rules
