module;

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <tuple>

export module scatter_span;
export import :iterator;

export namespace mem {

template<typename T>
class scatter_span
{

public:
  constexpr scatter_span<T>(std::initializer_list<std::span<T>> p_il)
    : m_spans(p_il.begin(), p_il.size())
    , m_start_pos(0)
    , m_final_len((p_il.end() - 1)->size())
  {
    m_total_length = calculate_length();
  }

  constexpr Iterator<T> begin()
  {
    return Iterator<T>({ .first = m_spans.data(),
                         .ptr = m_spans.data(),
                         .last = m_spans.data() + m_spans.size() - 1,
                         .start_pos = m_start_pos,
                         .final_len = m_final_len });
  }
  constexpr Iterator<T> end()
  {
    return Iterator<T>({ .first = m_spans.data(),
                         .ptr = m_spans.data() + m_spans.size(),
                         .last = m_spans.data() + m_spans.size() - 1,
                         .start_pos = m_start_pos,
                         .final_len = m_final_len });
  }

  struct sub_scatter_span_args
  {
    size_t offset;
    size_t count = std::dynamic_extent;
  };

  constexpr scatter_span<T> sub_scatter_span(sub_scatter_span_args const p_args)
  {
    std::ignore = p_args;
  }

  [[nodiscard]] constexpr size_t length() const
  {
    return m_total_length;
  }

protected:
  constexpr scatter_span<T>(std::span<std::span<T>> p_spans)
    : m_spans(p_spans)
    , m_final_len((p_spans.end() - 1)->size())
  {
  }

  std::span<std::span<T const>> m_spans;
  size_t m_start_pos;
  size_t m_final_len;
  size_t m_total_length;

private:
  size_t calculate_length()
  {
    size_t res = 0;
    for (auto const& s : m_spans) {
      res += s.size();
    }

    return res;
  }
};

template<typename T, size_t N>
class scatter_array : scatter_span<T>
{
  scatter_array<T, N>(std::initializer_list<std::span<T>> p_il)
    : m_internal_arr(p_il.begin(), p_il.size())
    , scatter_span<T>(std::span(m_internal_arr))
  {
  }

  std::array<std::span<T>, N> m_internal_arr;
};

}  // namespace mem
