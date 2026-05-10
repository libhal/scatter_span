module;

#include <array>
#include <cstddef>
#include <span>

export module scatter_span;

export namespace mem {

// Placeholder for scatter_span data structure

class scatter_span
{};

template<typename T, size_t N>
class scatter_array : scatter_span
{
  std::array<std::span<T>, N> m_internal_arr;
};
}  // namespace mem
