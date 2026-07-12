// Copyright 2026 Madeline Schneider and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

#include <array>
#include <cstddef>
#include <initializer_list>
#include <span>

#if defined(__has_cpp_attribute)
#if __has_cpp_attribute(clang::lifetimebound)
#define MEM_LIFETIMEBOUND clang::lifetimebound
#elif __has_cpp_attribute(msvc::lifetimebound)
#define MEM_LIFETIMEBOUND msvc::lifetimebound
#endif
#endif

#ifndef MEM_LIFETIMEBOUND
// TODO(gcc): no equivalent attribute as of GCC 15. When one ships, add:
// #elif __has_cpp_attribute(gnu::lifetime_bound)
// #    define MEM_LIFETIMEBOUND [[gnu::lifetime_bound]]
// above, before this fallback.
#define MEM_LIFETIMEBOUND
#endif

export module scatter_span;

namespace mem::detail {
template<typename T, size_t N>
struct scatter_array_storage
{
  std::array<std::span<T>, N> m_internal_arr;
};
};  // namespace mem::detail

namespace mem {

export template<typename T>
class scatter_span;

/**
 * @brief Forward iterator over the constituent spans of a scatter_span.
 *
 * @details Each dereference yields a @c std::span<T const> representing one
 * contiguous segment.
 *
 * The current pointer element is cached allowing for reads via the @c operator*
 * and @c operator-> to be constant time.
 *
 * @tparam T Element type of the underlying spans.
 */
export template<typename T>
class scatter_span_iterator
{
public:
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::span<T>;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;

  constexpr value_type get() const
  {
    // NOTE: the code doesn't check of m_index @ size or beyond since its UB to
    // dereference an iterator represented by `end()` and beyond. If contracts
    // are available, then a contract assert checks for this.
#if defined(__cpp_contracts)
    contract_assert(m_index >= m_scatter_span.m_spans.size());
#endif

    auto const last_element_index = m_scatter_span->m_spans.size() - 1;
    auto selected_span = m_scatter_span->m_spans[m_index];
    if (m_index == 0) {
      selected_span = selected_span.subspan(m_scatter_span->m_start_pos);
    }
    if (m_index == last_element_index) {
      // NOTE: This may seem invalid, but bare in mind that the invariant of
      // `sub_scatter_span` and other APIs is to ensures that the m_final_len is
      // always valid, even in the case of a scatter span with a single element.
      selected_span = selected_span.first(m_scatter_span->m_final_len);
    }
    return selected_span;
  }

  /**
   * @brief Returns the trimmed span view for the current position.
   * @return A const reference to the cached @c std::span<T const>.
   */
  constexpr value_type operator*() const
  {
    return get();
  }

  /**
   * @brief Returns a proxy object that allows member access to the current
   * span.
   */
  constexpr auto operator->() const
  {
    struct arrow_operator_proxy
    {
      // Cached span to be pull an address from
      value_type m_span;

      constexpr explicit arrow_operator_proxy(value_type p_span) noexcept
        : m_span(p_span)
      {
      }

      constexpr value_type const* operator->() const
      {
        // NOTE: `operator->` returns a temporary, this object, which will
        // outlive the expression, allowing this to be valid.
        return &m_span;
      }
    };
    return arrow_operator_proxy(get());
  }

  /**
   * @brief Prefix increment. Advances to the next span and updates the cache.
   * @return Reference to @c *this after advancing.
   */
  constexpr scatter_span_iterator& operator++()
  {
    ++m_index;
    return *this;
  }

  /**
   * @brief Postfix increment. Returns a copy of the iterator before advancing.
   * @return Copy of @c *this prior to the increment.
   */
  constexpr scatter_span_iterator operator++(int)
  {
    auto tmp = *this;
    ++(*this);
    return tmp;
  }

  /**
   * @brief Prefix decrement. Retreats to the previous span and updates the
   * cache.
   * @return Reference to @c *this after retreating.
   */
  constexpr scatter_span_iterator& operator--()
  {
    --m_index;
    return *this;
  }

  /**
   * @brief Postfix decrement. Returns a copy of the iterator before retreating.
   * @return Copy of @c *this prior to the decrement.
   */
  constexpr scatter_span_iterator operator--(int)
  {
    auto tmp = *this;
    --(*this);
    return tmp;
  }

  /**
   * @brief Returns a new iterator advanced by @p p_delta spans.
   * @param p_delta Number of spans to advance.
   * @return New iterator pointing @p p_delta spans ahead.
   */
  constexpr scatter_span_iterator operator+(difference_type p_delta) const
  {
    auto tmp = *this;
    tmp.m_index += p_delta;
    return tmp;
  }

  /**
   * @brief Returns a new iterator retreated by @p p_delta spans.
   * @param p_delta Number of spans to retreat.
   * @return New iterator pointing @p p_delta spans behind.
   */
  constexpr scatter_span_iterator operator-(difference_type p_delta) const
  {
    auto tmp = *this;
    tmp.m_index -= p_delta;
    return tmp;
  }

  /**
   * @brief Returns the signed span distance between two iterators.
   * @param p_other The iterator to subtract.
   * @return Number of spans from @p p_other to @c *this.
   */
  constexpr difference_type operator-(
    scatter_span_iterator const& p_other) const
  {
    return m_index - p_other.m_index;
  }

  /**
   * @brief Commutative addition: advances @p it by @p n spans.
   * @param n   Number of spans to advance.
   * @param it  Iterator to advance.
   * @return New iterator pointing @p n spans ahead of @p it.
   */
  friend constexpr scatter_span_iterator operator+(
    difference_type p_delta,
    scatter_span_iterator const& p_iterator)
  {
    return p_iterator + p_delta;
  }

  /**
   * @brief Equality comparison by underlying pointer.
   * @param p_other Iterator to compare against.
   * @return @c true if both iterators point to the same span.
   */
  constexpr bool operator==(scatter_span_iterator const& p_other) const
  {
    return (m_scatter_span == p_other.m_scatter_span) and
           (m_index == p_other.m_index);
  }

  /**
   * @brief Inequality comparison by underlying pointer.
   * @param p_other Iterator to compare against.
   * @return @c true if the iterators point to different spans.
   */
  constexpr bool operator!=(scatter_span_iterator const& p_other) const
  {
    return not(*this == p_other);
  };

private:
  friend class scatter_span<T>;
  /**
   * @brief Constructs an iterator for @p p_ssp pointing at @p p_ptr.
   * @param p_ssp Parent scatter_span providing trim metadata.
   * @param p_ptr Pointer into @p p_ssp's internal span array.
   * @note The cache is primed immediately on construction.
   */
  constexpr explicit scatter_span_iterator(scatter_span<T> const* p_ssp,
                                           size_t p_initial_index)
    : m_scatter_span(p_ssp)
    , m_index(p_initial_index)
  {
  }

  scatter_span<T> const* m_scatter_span = nullptr;
  size_t m_index = 0;
};

/**
  @brief A concept for types that can be in some way, converted to a @c
  std::span<T>. Used primarily to denote container types.

  @tparam T - the underlying type for a given spanable container.
 */
export template<typename T>
concept spanable = requires(T& t) {
  { std::span(t) };
};

/**
 * @brief Non-owning immutable view over a collection of disjoint memory spans.
 *
 * @details Works similarly to @c std::span, but the viewed elements do not
 * need to be contiguous in memory. Each constituent chunk is a separate
 * @c std::span<T const>. Iterating yields each chunk a span in order.
 *
 * Prefer @c scatter_span for ad-hoc, call-site construction. Commonly used for
 * when forwarding discontiguous buffers to an API without a heap allocation or
 * memcpy. Prefer @c scatter_array when the span set is reused or must outlive
 * the call site.
 *
 * @par Example
 * @code
 * void send_packet(mem::scatter_span<std::byte> payload);
 *
 * static std::array<std::byte, 4 const> header = { ... };
 * std::array<std::byte, 8> body   = { ... };
 * std::array<std::byte, 2> crc    = { ... };
 *
 * // Three discontiguous buffers assembled into one logical packet inline,
 * // with no heap allocation or memcpy:
 * send_packet({ header, body, crc });
 * @endcode
 *
 * @tparam T Element type. Elements are always accessed as @c T const.
 */
export template<typename T>
class scatter_span
{
public:
  /**
   * @brief Constructs a scatter_span directly from a span-of-spans.
   *
   * @details Used by @c scatter_array to hand its internal storage to the base
   * class. Assumes full extents of the first and last chunks with no trimming.
   *
   * @param p_spans View over the array of chunk spans.
   */
  constexpr scatter_span(std::span<std::span<T> const> p_spans
                         [[MEM_LIFETIMEBOUND]])
    : m_spans(p_spans)
    , m_final_len(p_spans.back().size())
  {
  }

  /**
   * @brief Constructs a scatter_span from a brace-enclosed list of spans.
   *
   * @details The first and last spans are recorded for trim purposes. An empty
   * initializer list produces a zero-length scatter_span.
   *
   * @param p_il Initializer list of @c std::span<T const> chunks, in logical
   *             order.
   */
  constexpr scatter_span(std::initializer_list<std::span<T>> p_spans
                         [[MEM_LIFETIMEBOUND]])
    : m_spans(p_spans.begin(), p_spans.end())
    , m_final_len((p_spans.end() - 1)->size())
  {
  }

  /**
   * @brief Construct an empty scatter span
   *
   */
  constexpr scatter_span() = default;

  /**
   * @brief Returns an iterator to the first span chunk.
   * @return @c scatter_span_iterator<T> pointing at the first chunk.
   */
  [[nodiscard]] constexpr scatter_span_iterator<T> begin() const&
  {
    return scatter_span_iterator<T>(this, 0);
  }

  /**
   * @brief Deleted overload preventing iteration over a temporary
   * @c scatter_array.
   *
   * @details @c scatter_array owns its span-array storage (@c m_internal_arr).
   * Calling @c begin() on an rvalue would produce an iterator that outlives the
   * @c scatter_array temporary destroying that storage, leaving the returned
   * iterator dangling into freed memory. This overload is deleted to force a
   * compile-time error at the call site instead of runtime undefined behavior.
   *
   * @par Example (rejected at compile time)
   * @code
   * // ERROR: use of deleted function
   * auto it = mem::scatter_array(a, b, c).begin();
   * @endcode
   *
   * @par Fix: bind to a named variable first
   * @code
   * auto ssa = mem::scatter_array(a, b, c);
   * auto it = ssa.begin();  // OK — ssa outlives `it`
   * @endcode
   */
  constexpr scatter_span_iterator<T> begin() const&& = delete;

  /**
   * @brief Returns a past-the-end iterator.
   * @return @c scatter_span_iterator<T> pointing one past the last chunk.
   */
  [[nodiscard]] constexpr scatter_span_iterator<T> end() const&
  {
    return scatter_span_iterator<T>(this, m_spans.size());
  }

  /**
   * @brief Deleted overload preventing end-iteration over a temporary
   * @c scatter_array.
   *
   * @details Mirrors @c begin() const&& = delete for the same reason: an
   * end-iterator produced from an rvalue @c scatter_array would reference
   * storage (@c m_internal_arr) that no longer exists once the temporary is
   * destroyed at the end of the full-expression.
   *
   * @par Example (rejected at compile time)
   * @code
   * // ERROR: use of deleted function
   * auto it = mem::scatter_array(a, b, c).end();
   * @endcode
   *
   * @par Fix: bind to a named variable first
   * @code
   * auto ssa = mem::scatter_array(a, b, c);
   * auto it = ssa.end();  // OK — ssa outlives `it`
   * @endcode
   */
  constexpr scatter_span_iterator<T> end() const&& = delete;

  /**
   * @brief Arguments for @c sub_scatter_span().
   * Offset - Logical element index to start from.
   * Count - Number of elements to include. (Defaults to all remaining)
   */
  struct sub_scatter_span_args
  {
    size_t offset = 0;
    size_t count = std::dynamic_extent;
  };

  /**
   * @brief Returns a sub-view of this scatter_span starting at a logical
   * element offset.
   *
   * @details Analogous to @c std::span::subspan. No data is copied; the
   * returned @c scatter_span references the same underlying memory with
   * adjusted trim values. If @p p_args.offset is at or beyond the total
   * length, an empty @c scatter_span is returned. If @p p_args.count exceeds
   * the remaining elements it is clamped to the available length.
   *
   * @par Example
   * @code
   * std::array<int, 3> a = { 1, 2, 3 };
   * std::array<int, 2> b = { 4, 5 };
   * std::array<int, 4> c = { 6, 7, 8, 9 };
   * mem::scatter_array<int, 3> ssa(a, b, c);  // length 9
   *
   * // Logical elements [2, 7) → { 3, 4, 5, 6, 7 }
   * auto sub = ssa.sub_scatter_span({ .offset = 2, .count = 5 });
   * @endcode
   *
   * @param p_args Offset and count describing the desired sub-range.
   * @return A new @c scatter_span<T> covering the requested elements.
   */
  constexpr scatter_span<T> sub_scatter_span(sub_scatter_span_args p_args) &
  {
    if (m_spans.size() == 1) {
      auto& the_only_span = m_spans[0];

      if (p_args.offset >= the_only_span.size()) {
        return {};
      }

      auto const clamped_length =
        std::min(p_args.count, the_only_span.size() - p_args.offset);

      return scatter_span<T>(
        {
          .start_pos = p_args.offset,
          .final_len = clamped_length,
        },
        m_spans);
    }

    auto const len = length();

    if (p_args.offset >= len) {
      return {};
    }

    if (p_args.count >= len - p_args.offset) {
      p_args.count = len - p_args.offset;
    }

    size_t cur_len = 0;
    size_t starting_span_idx = 0;
    size_t effective_span_size = 0;
    bool boundary = false;
    for (auto s : *this) {
      effective_span_size = s.size();
      cur_len += effective_span_size;
      if (cur_len > p_args.offset) {
        break;
      } else if (cur_len == p_args.offset) {
        starting_span_idx += 1;
        boundary = true;
        break;
      }

      starting_span_idx += 1;
    }

    size_t start_pos = 0;
    if (!boundary) {
      start_pos = p_args.offset - (cur_len - effective_span_size);
    }
    size_t span_idx = 0;
    cur_len = 0;
    auto const considered_spans = m_spans.subspan(starting_span_idx);
    size_t const adjusted_count = p_args.count + start_pos;
    for (std::span<T const> s : considered_spans) {
      cur_len += s.size();
      if (cur_len >= adjusted_count) {
        break;
      }
      span_idx += 1;
    }

    if (cur_len == adjusted_count) {
      size_t const final_len =
        (span_idx == 0) ? p_args.count : considered_spans[span_idx].size();
      return scatter_span<T>({ .start_pos = start_pos, .final_len = final_len },
                             considered_spans.subspan(0, span_idx + 1));
    }

    auto const final_offset =
      adjusted_count - (cur_len - considered_spans[span_idx].size());
    return scatter_span<T>(
      { .start_pos = start_pos, .final_len = final_offset },
      considered_spans.subspan(0, span_idx + 1));
  }

  /**
   * @brief Deleted overload preventing sub-view creation from a temporary
   * @c scatter_array.
   *
   * @details @c sub_scatter_span() returns a @c scatter_span<T> that views
   * @c scatter_array's internally owned span array. If called on an rvalue,
   * the returned @c scatter_span would reference storage destroyed at the end
   * of the full-expression that created the temporary @c scatter_array,
   * leaving the result dangling. This overload is deleted so misuse fails to
   * compile rather than producing a sub-view that reads freed memory.
   *
   * @par Example (rejected at compile time)
   * @code
   * // ERROR: use of deleted function
   * auto sub = mem::scatter_array(a, b, c).sub_scatter_span({ .count = 5 });
   * @endcode
   *
   * @par Fix: bind to a named variable first
   * @code
   * auto ssa = mem::scatter_array(a, b, c);
   * auto sub = ssa.sub_scatter_span({ .count = 5 });  // OK — ssa outlives sub
   * @endcode
   *
   * @note This restriction applies only to @c scatter_array, which owns its
   * span storage. @c scatter_span itself is safe to construct and use as a
   * temporary, since the data it views lives in the caller's expression, not
   * in the @c scatter_span object.
   */
  constexpr scatter_span<T> sub_scatter_span(sub_scatter_span_args p_args) && =
    delete;

  /**
   * @brief Returns the total number of logical elements across all chunks.
   *
   * @return Total element count.
   */
  [[nodiscard]] size_t length() const
  {
    if (m_spans.size() <= 1) {
      return m_final_len;
    }

    size_t res = m_spans[0].size() - m_start_pos;

    auto const second_to_last_element = m_spans.size() - 2;
    for (auto const& s : m_spans.subspan(1, second_to_last_element)) {
      res += s.size();
    }

    return res + m_final_len;
  }

  friend class scatter_span_iterator<T>;

protected:
  /**
   * @brief Trim metadata produced by @c sub_scatter_span().
   */
  struct position_data
  {
    size_t start_pos =
      0;  ///< Index into the first chunk to begin reading from.
    size_t final_len = 0;  ///< Number of elements to read from the last chunk.
  };

  /**
   * @brief Constructs a trimmed scatter_span with explicit position metadata.
   *
   * @details Used internally by @c sub_scatter_span() to produce a sub-view
   * without copying data.
   *
   * @param p_pos   Trim parameters for the first and last chunks.
   * @param p_spans The subset of chunk spans to reference.
   */
  constexpr scatter_span(position_data p_pos,
                         std::span<std::span<T> const> p_spans)
    : m_spans(p_spans)
    , m_start_pos(p_pos.start_pos)
    , m_final_len(p_pos.final_len)
  {
  }

  std::span<std::span<T> const> m_spans{};
  size_t m_start_pos = 0;
  size_t m_final_len = 0;
};

/**
 * @brief Owning scatter span. Stores the chunk span array internally and
 * exposes the full @c scatter_span interface.
 *
 * @details Unlike @c scatter_span, @c scatter_array owns the @c std::span
 * descriptors (not the underlying data). This makes it safe to store as a
 * member or return from a function without dangling. The number of chunks @p N
 * is fixed at compile time; the backing arrays themselves remain external and
 * must outlive the @c scatter_array.
 *
 * Prefer @c scatter_array when the same set of chunks is iterated or
 * sub-spanned multiple times across a lifetime (e.g. a reusable packet
 * descriptor). Prefer @c scatter_span for single-use, call-site construction.
 *
 * @par Example
 * @code
 * std::array<int, 3> first  = { 1, 2, 3 };
 * std::array<int, 2> second = { 4, 5 };
 * std::array<int, 4> third  = { 6, 7, 8, 9 };
 *
 * // N deduced as 3 via CTAD
 * mem::scatter_array ssa(first, second, third);
 *
 * // Reuse: take different sub-views without re-specifying chunks each time
 * auto head = ssa.sub_scatter_span({ .count = 5 });       // { 1,2,3,4,5 }
 * auto tail = ssa.sub_scatter_span({ .offset = 5 });      // { 6,7,8,9 }
 * auto mid  = ssa.sub_scatter_span({ .offset = 2, .count = 5 }); // { 3,4,5,6,7
 * }
 * @endcode
 *
 * @tparam T Element type.
 * @tparam N Number of chunk spans stored internally.
 */
export template<typename T, size_t N>
class scatter_array
  : private detail::scatter_array_storage<T, N>
  , public scatter_span<T>
{
public:
  /**
   * @brief Constructs a scatter_array from any number of spanable containers.
   *
   * @details Each argument is converted to @c std::span<T const> and stored in
   * the internal array. The base @c scatter_span is then initialized to view
   * that array. @p N must equal @c sizeof...(p_spans).
   *
   * @tparam Spans Pack of spanable container types (e.g. @c std::array,
   *               @c std::vector).
   * @param p_spans Containers whose spans form the logical sequence.
   */
  template<spanable... Spans>
  constexpr scatter_array(Spans&&... p_spans)
    : detail::scatter_array_storage<T,
                                    N>{ .m_internal_arr = { std::span<T>(
                                          p_spans)... }, }
    , scatter_span<T>(std::span<std::span<T> const>(this->m_internal_arr))
  {
  }
};

/**
 * @brief Deduction guide for @c scatter_array.
 *
 * @details Deduces @c T from the value type of the first argument and @c N
 * from the total argument count. All arguments must have the same element type.
 */
export template<spanable First, spanable... Spans>
  requires(
    std::same_as<
      typename decltype(std::span(std::declval<First&>()))::value_type,
      typename decltype(std::span(std::declval<Spans&>()))::value_type> &&
    ...)
scatter_array(First& p_first, Spans&&... p_spans) -> scatter_array<
  typename std::remove_reference_t<decltype(p_first)>::value_type,
  sizeof...(p_spans) + 1>;
}  // namespace mem
