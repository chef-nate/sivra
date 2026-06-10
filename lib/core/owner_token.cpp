#include <sivra/core/owner_token.hpp>

#include <atomic>

namespace sivra::core {

owner_token owner_token_source::next() {
  static std::atomic<std::uint64_t> next_value = 1;
  return owner_token::unsafe_from_value(next_value.fetch_add(1, std::memory_order_relaxed));
}

} // namespace sivra::core
