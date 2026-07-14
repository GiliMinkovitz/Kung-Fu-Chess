#include "piece_factory.h"

namespace kfc {

Piece PieceFactory::create(PieceColor color, PieceKind kind, Position cell, PieceState state) {
    return Piece{next_id_++, color, kind, cell, state};
}

}  // namespace kfc
