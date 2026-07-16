#pragma once

#include <cstdint>

namespace kfc {

inline constexpr char kWhiteColor = 'w';
inline constexpr char kBlackColor = 'b';
inline constexpr char kEmptyToken = '.';

inline constexpr char kKingType = 'K';
inline constexpr char kQueenType = 'Q';
inline constexpr char kRookType = 'R';
inline constexpr char kBishopType = 'B';
inline constexpr char kKnightType = 'N';
inline constexpr char kPawnType = 'P';

inline constexpr std::int64_t kMoveDurationMs = 1000;
inline constexpr std::int64_t kJumpDurationMs = 1000;
inline constexpr std::int64_t kLongRestDurationMs = 1000;
inline constexpr std::int64_t kShortRestDurationMs = 1000;
inline constexpr std::int64_t kTargetFrameMs = 16;

inline constexpr const char* kBoardSectionHeader = "Board:";
inline constexpr const char* kCommandsSectionHeader = "Commands:";
inline constexpr const char* kPrintBoardCommand = "print board";

inline constexpr const char* kErrorUnknownToken = "ERROR UNKNOWN_TOKEN";
inline constexpr const char* kErrorRowWidthMismatch = "ERROR ROW_WIDTH_MISMATCH";

}  // namespace kfc
