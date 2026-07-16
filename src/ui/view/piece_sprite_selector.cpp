#include "ui/view/piece_sprite_selector.h"

namespace kfc {

PieceSpriteSelection PieceSpriteSelector::select(const PieceView& /*piece*/) const {
    return {"idle", 1};
}

}  // namespace kfc
