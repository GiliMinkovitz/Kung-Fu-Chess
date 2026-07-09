#pragma once

#include "core/piece.h"

namespace kfc {

class PieceFactory {
public:
    explicit PieceFactory(Piece::Id start_id = 1) : next_id_(start_id) {}

    [[nodiscard]] Piece create(PieceColor color, PieceKind kind, Position cell,
                               PieceState state = PieceState::Idle);

private:
    Piece::Id next_id_;
};

}  // namespace kfc
