module;
#include <cstddef>
#include <iterator>
#include <span>

export module scatter_span:iterator;

namespace mem {

template<typename T>
struct Iterator  // NOLINT
{
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::span<T const>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type const*;
  using reference = value_type const&;

  constexpr void update_cache() const
  {
    if (m_first == m_ptr) {
      m_subspan_cache = m_ptr->last(m_start_pos);
    } else if (m_ptr == m_last) {
      m_subspan_cache = m_ptr->first(m_final_len);
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
    update_cache();
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
    update_cache();
    return tmp;
  }

  constexpr bool operator<=>(Iterator const&) const = default;

private:
  struct iterator_args
  {
    pointer first;
    pointer ptr;
    pointer last;
    size_t start_pos;
    size_t final_len;
  };

  constexpr explicit Iterator(iterator_args const p_args)
    : m_first(p_args.first)
    , m_ptr(p_args.ptr)
    , m_last(p_args.last)
    , m_start_pos(p_args.start_pos)
    , m_final_len(p_args.start_pos)
  {
  }

  pointer m_first;
  pointer m_last;
  pointer m_ptr;

  size_t const m_start_pos;
  size_t const m_final_len;
  std::span<T const> m_subspan_cache;
};
}  // namespace mem
