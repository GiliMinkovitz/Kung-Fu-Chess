#pragma once

#ifdef KFC_TEST_BUILD

namespace kfc::test {

struct GameMessageParserTestHooks {
    static inline bool force_invalid_piece_descriptor = false;

    static void reset() { force_invalid_piece_descriptor = false; }
};

}  // namespace kfc::test

#endif
