#include "app/observability/correlation_id.h"

#include <atomic>
#include <random>
#include <sstream>

namespace kfc::app::observability {

namespace {

thread_local std::string g_correlation_id;

std::string random_suffix() {
    static std::atomic<unsigned int> counter{0};
    const unsigned int value = counter.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream stream;
    stream << std::hex << value;
    return stream.str();
}

}  // namespace

std::string generate_correlation_id() {
    return "kfc-" + random_suffix() + "-" + random_suffix();
}

const std::string& current_correlation_id() {
    return g_correlation_id;
}

void set_correlation_id(const std::string correlation_id) {
    g_correlation_id = correlation_id;
}

void clear_correlation_id() {
    g_correlation_id.clear();
}

CorrelationScope::CorrelationScope(const std::string correlation_id) {
    if (!g_correlation_id.empty()) {
        previous_ = g_correlation_id;
        had_previous_ = true;
    }
    g_correlation_id = correlation_id;
}

CorrelationScope::~CorrelationScope() {
    if (had_previous_) {
        g_correlation_id = previous_;
    } else {
        g_correlation_id.clear();
    }
}

}  // namespace kfc::app::observability
