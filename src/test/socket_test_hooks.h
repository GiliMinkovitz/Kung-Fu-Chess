#pragma once

#ifdef KFC_TEST_BUILD

#include <boost/beast/core.hpp>

#include <optional>

namespace kfc::test {

struct SocketTestHooks {
    static inline std::optional<boost::beast::error_code> next_avail_error;
    static inline std::optional<boost::beast::error_code> next_read_error;
    static inline std::optional<boost::beast::error_code> next_write_error;
    static inline std::optional<boost::beast::error_code> next_peek_error;
    static inline std::optional<std::size_t> forced_peeked;
    static inline bool force_non_blocking_error = false;
    static inline bool force_peek_success_with_data = false;
    static inline std::optional<std::string> forced_read_message;

    static void reset() {
        next_avail_error.reset();
        next_read_error.reset();
        next_write_error.reset();
        next_peek_error.reset();
        forced_peeked.reset();
        force_non_blocking_error = false;
        force_peek_success_with_data = false;
        forced_read_message.reset();
    }
};

}  // namespace kfc::test

#endif
