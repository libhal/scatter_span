module;

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>

export module scatter_span:core;

namespace mem::detail {
template<typename T, size_t N>
struct scatter_array_storage
{
  std::array<std::span<T const>, N> m_internal_arr;
};
};  // namespace mem::detail

export namespace mem {

template<typename T>
class scatter_span;

template<typename T>
struct Iterator  // NOLINT
{
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::span<T const>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type const*;
  using reference = value_type const&;

  constexpr void update_cache()
  {
    auto const* first = m_ssp->m_spans.data();
    auto const* last = m_ssp->m_spans.data() + m_ssp->m_spans.size() - 1;
    if (m_ptr > last) {
      return;
    } else if (first == last) {
      m_subspan_cache = m_ptr->subspan(m_ssp->m_start_pos, m_ssp->m_final_len);
    } else if (m_ptr == first) {
      m_subspan_cache = m_ptr->subspan(m_ssp->m_start_pos);
    } else if (m_ptr == last) {
      m_subspan_cache = m_ptr->first(m_ssp->m_final_len);
    } else {
      m_subspan_cache = *m_ptr;
    }
  }

  constexpr reference operator*() const
  {
    return m_subspan_cache;
  }
  constexpr pointer operator->() const
  {
    return &m_subspan_cache;
  }

  constexpr Iterator& operator++()
  {
    ++m_ptr;
    update_cache();
    return *this;
  }
  constexpr Iterator operator++(int)
  {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  constexpr Iterator& operator--()
  {
    --m_ptr;
    update_cache();
    return *this;
  }
  constexpr Iterator operator--(int)
  {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  constexpr Iterator operator+(difference_type n) const
  {
    auto tmp = *this;
    tmp.m_ptr += n;
    tmp.update_cache();
    return tmp;
  }

  constexpr Iterator operator-(difference_type n) const
  {
    auto tmp = *this;
    tmp.m_ptr -= n;
    tmp.update_cache();
    return tmp;
  }

  constexpr difference_type operator-(Iterator const& other) const
  {
    return m_ptr - other.m_ptr;
  }

  friend constexpr Iterator operator+(difference_type n, Iterator const& it)
  {
    return it + n;
  }

  constexpr bool operator==(Iterator const& other) const
  {
    return m_ptr == other.m_ptr;
  }

  constexpr bool operator!=(Iterator const& other) const
  {
    return m_ptr != other.m_ptr;
  };

  constexpr explicit Iterator(scatter_span<T> const& p_ssp, pointer p_ptr)
    : m_ssp(&p_ssp)
    , m_ptr(p_ptr)
  {
    update_cache();
  }

  scatter_span<T> const* m_ssp;
  pointer m_ptr;
  std::span<T const> m_subspan_cache;
};

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
    , m_final_len(p_il.size() == 0 ? 0 : (p_il.end() - 1)->size())
  {
  }

  [[nodiscard]] constexpr Iterator<T> begin() const
  {
    return Iterator<T>(*this, m_spans.data());
  }

  [[nodiscard]] constexpr Iterator<T> end() const
  {
    return Iterator<T>(*this, m_spans.data() + m_spans.size());
  }

  struct sub_scatter_span_args
  {
    size_t offset = 0;
    size_t count = std::dynamic_extent;
  };

  constexpr scatter_span<T> sub_scatter_span(sub_scatter_span_args p_args)
  {
    auto len = length();

    if (p_args.offset >= len) {
      return scatter_span<T>({});
    }

    if (p_args.count >= len - p_args.offset) {
      p_args.count = len - p_args.offset;
    }

    size_t cur_len = 0;
    size_t starting_span_idx = 0;
    size_t effective_span_size = 0;
    bool boundry = false;
    for (auto s : *this) {
      effective_span_size = s.size();
      cur_len += effective_span_size;
      if (cur_len > p_args.offset) {
        break;
      } else if (cur_len == p_args.offset) {
        starting_span_idx += 1;
        boundry = true;
        break;
      }

      starting_span_idx += 1;
    }

    size_t start_pos =
      boundry ? 0 : p_args.offset - (cur_len - effective_span_size);
    size_t span_idx = 0;
    cur_len = 0;
    auto considered_spans = m_spans.subspan(starting_span_idx);
    size_t adjusted_count = p_args.count + start_pos;
    for (std::span<T const> s : considered_spans) {
      cur_len += s.size();
      if (cur_len >= adjusted_count) {
        break;
      }
      span_idx += 1;
    }

    if (cur_len == adjusted_count) {
      size_t final_len = (span_idx == 0) ? p_args.count
                                         : considered_spans[span_idx].size();
      return scatter_span<T>({ .start_pos = start_pos, .final_len = final_len },
                             considered_spans.subspan(0, span_idx + 1));
    }

    auto final_offset =
      adjusted_count - (cur_len - considered_spans[span_idx].size());
    return scatter_span<T>(
      { .start_pos = start_pos, .final_len = final_offset },
      considered_spans.subspan(0, span_idx + 1));
  }

  [[nodiscard]] size_t length() const
  {
    if (m_spans.size() <= 1) {
      return m_final_len;
    }

    size_t res = m_spans[0].size() - m_start_pos;

    for (auto const& s : m_spans.subspan(1, m_spans.size() - 2)) {
      res += s.size();
    }

    return res + m_final_len;
  }

  friend struct Iterator<T>;

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
