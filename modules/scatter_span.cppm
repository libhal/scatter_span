module;

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>

export module scatter_span;
export import :iterator;

namespace mem::detail {
template<typename T, size_t N>
struct scatter_array_storage
{
  std::array<std::span<T const>, N> m_internal_arr;
};
};  // namespace mem::detail

export namespace mem {

template<typename T>
concept spanable = requires(T& t) {
  { std::span(t) };
};

template<typename T>
class scatter_span
{

public:
  constexpr scatter_span<T>(std::initializer_list<std::span<T const>> p_il)
    : m_spans(p_il.begin(), p_il.size())
    , m_start_pos(0)
    , m_final_len((p_il.end() - 1)->size())
  {
  }

  constexpr Iterator<T> begin() const
  {
    return Iterator<T>({ .first = m_spans.data(),
                         .ptr = m_spans.data(),
                         .last = m_spans.data() + m_spans.size() - 1,
                         .start_pos = m_start_pos,
                         .final_len = m_final_len });
  }

  constexpr Iterator<T> end() const
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

  constexpr scatter_span<T> sub_scatter_span(sub_scatter_span_args p_args)
  {
    auto len = length();
    if (len <= p_args.count) {
      return *this;
    }

    // TODO: offset

    size_t cur_len = 0;
    size_t span_idx = 0;
    for (std::span<T const> s : *this) {
      cur_len += s.size();
      if (cur_len >= p_args.count) {
        break;
      }
      span_idx += 1;
    }

    // TODO: fill in starting positions with correct values

    if (cur_len == p_args.count) {
      return scatter_span<T>(
        { .start_pos = 0, .final_len = m_spans[span_idx].size() },
        m_spans.subspan(0, span_idx + 1));
    }

    // this calculation is wrong, figure out a way to properly
    // calculate where the final length should be. ie if there
    // are 9 elements in a 3-2-3 configuration, and 6 are asked
    // for the final length should be 1 as we only need to
    // first element from the final array. maybe can do
    // (cur_len - args.count) - final_array_size?
    auto final_offset = cur_len - p_args.count;
    return scatter_span<T>({ .start_pos = 0, .final_len = final_offset },
                           m_spans.subspan(0, span_idx + 1));
  }

  [[nodiscard]] size_t length() const
  {
    size_t res = 0;
    for (auto const& s : m_spans) {
      res += s.size();
    }

    return res;
  }

protected:
  constexpr scatter_span(std::span<std::span<T const> const> p_spans)
    : m_spans(p_spans)
    , m_start_pos(0)
    , m_final_len((p_spans.end() - 1)->size())
  {
  }

  std::span<std::span<T const> const> m_spans;
  size_t m_start_pos;
  size_t m_final_len;

  struct position_data
  {
    size_t start_pos = 0;
    size_t final_len = 0;
  };

  constexpr scatter_span<T>(position_data p_pos,
                            std::span<std::span<T const> const> p_spans)
    : m_spans(p_spans)
    , m_start_pos(p_pos.start_pos)
    , m_final_len(p_pos.final_len)
  {
  }
};

template<typename T, size_t N>
class scatter_array
  : private detail::scatter_array_storage<T, N>
  , public scatter_span<T>
{
public:
  template<spanable... Spans>
  constexpr scatter_array(Spans&&... p_spans)
    : detail::scatter_array_storage<T,
                                    N>{ .m_internal_arr = { std::span<T const>(
                                          p_spans)... } }
    , scatter_span<T>(this->m_internal_arr)
  {
  }
};

template<spanable First, spanable... Spans>
  requires(
    std::same_as<
      typename decltype(std::span(std::declval<First&>()))::value_type,
      typename decltype(std::span(std::declval<Spans&>()))::value_type> &&
    ...)
scatter_array(First& p_first, Spans&&... p_spans) -> scatter_array<
  typename std::remove_reference_t<decltype(p_first)>::value_type,
  sizeof...(p_spans) + 1>;

}  // namespace mem
