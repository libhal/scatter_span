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

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <functional>
#include <ostream>
#include <print>
#include <span>
#include <utility>
#include <vector>

#include <boost/ut.hpp>

import scatter_span;

namespace mem {
/* Prints the logical element sequence of a scatter_span, e.g. `[1, 2, 3]`.
 * Lets boost::ut print the operands of a failed `expect` involving a
 * scatter_span.
 */
template<typename T>
std::ostream& operator<<(std::ostream& p_os, scatter_span<T> const& p_ssp)
{
  p_os << "[ ";
  for (auto chunk : p_ssp) {
    p_os << "[ ";
    for (size_t i = 0; i < chunk.size(); i++) {
      p_os << chunk[i];
      if (i != chunk.size() - 1) {
        p_os << ", ";
      }
    }
    p_os << " ]";
  }
  p_os << " ]";
  return p_os;
}

/* A helper function to compare two scatter_spans. This is only useful for
 * testing
 */
template<typename T>
bool operator==(scatter_span<T> const& lhs, scatter_span<T> const& rhs)
{
  if (lhs.length() != rhs.length()) {
    return false;
  }

  if (lhs.length() == 0) {
    return true;
  }

  std::vector<T> lhs_vec;
  std::vector<T> rhs_vec;

  for (auto s : lhs) {
    lhs_vec.append_range(s);
  }

  for (auto s : rhs) {
    rhs_vec.append_range(s);
  }

  return std::ranges::equal(lhs_vec, rhs_vec);
}
}  // namespace mem

namespace {
void basic_scatter_span_tests()
{
  using namespace boost::ut;
  using namespace mem;

  std::array<int, 3> first = { 1, 2, 3 };
  std::array<int, 2> second{ 4, 5 };
  std::array<int, 4> third = { 6, 7, 8, 9 };
  std::array<int, 9> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  "empty span should have zero length"_test = [&] {
    scatter_span<int> empty_scatter_span{};
    expect(that % empty_scatter_span.length() == 0);
  };

  "constructor & length"_test = [&] {
    scatter_array<int, 3> ssp(first, second, third);
    expect(that % ssp.length() == 9);

    expect(that % ssp == scatter_span<int>{ expected });
  };

  "api_lifetime"_test = [&] {
    auto mock_api = [&first, &second, &third](scatter_span<int> p_ssp) {
      std::array<int, 9> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
      scatter_array<int, 1> expected_ssp(expected);
      expect(that % expected_ssp == p_ssp);

      auto ssp_it = p_ssp.begin();
      expect(that % first.data() == ssp_it->data());
      ssp_it++;
      expect(that % second.data() == ssp_it->data());
      ssp_it++;
      expect(that % third.data() == ssp_it->data());
    };

    mock_api({ first, second, third });
  };

  "sub_scatter_span()"_test = [&] {
    scatter_array ssa = { first, second, third };

    auto subssp = ssa.sub_scatter_span({ .offset = 0, .count = 5 });
    expect(that % subssp == scatter_span<int>{ std::span(expected).first(5) });

    auto uneven_ssp = ssa.sub_scatter_span({ .offset = 0, .count = 6 });
    expect(that % uneven_ssp ==
           scatter_span<int>{ std::span(expected).first(6) });

    auto uneven_ssp_two = ssa.sub_scatter_span({ .offset = 0, .count = 2 });
    expect(that % uneven_ssp_two ==
           scatter_span<int>{ std::span(expected).first(2) });

    auto offset_even_ssp = ssa.sub_scatter_span({ .offset = 2 });
    expect(that % offset_even_ssp.length() == (ssa.length() - 2));
    expect(that % offset_even_ssp ==
           scatter_span<int>{ std::span(expected).subspan(2) });

    auto uneven_offset_ssp = ssa.sub_scatter_span({ .offset = 3, .count = 5 });
    expect(that % uneven_offset_ssp ==
           scatter_span<int>{ std::span(expected).subspan(3, 5) });

    auto offset_greater_than_len = ssa.sub_scatter_span({ .offset = 9 });
    expect(that % offset_greater_than_len == scatter_span<int>{});
  };

  "sub_scatter_span() empty span"_test = [&] {
    scatter_span<int> ssa{};

    auto subssp = ssa.sub_scatter_span({ .offset = 0, .count = 5 });

    expect(that % 0uz == ssa.length());
    expect(that % subssp == scatter_span<int>{});
  };

  "sub_scatter_span() with range that exceeds space"_test = [&] {
    scatter_array ssa = { first, second, third };

    auto subssp = ssa.sub_scatter_span({ .offset = 20, .count = 5 });

    expect(that % 0uz == subssp.length());
    expect(that % scatter_span<int>{} == subssp);
  };

  "sub_scatter_span() single element span"_test = [&] {
    scatter_array ssa = { third };
    // Pick the middle two elements of the third array
    auto subssp = ssa.sub_scatter_span({ .offset = 1, .count = 2 });
    scatter_array<int, 1> const expected({ std::span(third).subspan(1, 2) });
    expect(that % subssp == expected);
  };

  "sub_scatter_span() of a trimmed single chunk view"_test = [&] {
    // Setup
    std::array<int, 6> data = { 1, 2, 3, 4, 5, 6 };
    scatter_array ssa = { data };
    auto trimmed = ssa.sub_scatter_span({ .offset = 2 });

    // Exercise
    auto sub = trimmed.sub_scatter_span({ .offset = 1 });

    // Verify
    expect(that % sub == scatter_span<int>{ std::span(data).subspan(3) });
  };

  "sub_scatter_span() of a view trimmed into its first chunk"_test = [&] {
    // Setup
    scatter_array ssa = { first, second, third };
    auto trimmed = ssa.sub_scatter_span({ .offset = 1 });

    // Exercise
    auto sub = trimmed.sub_scatter_span({ .offset = 1, .count = 3 });

    // Verify
    expect(that % sub ==
           scatter_span<int>{ std::span(expected).subspan(2, 3) });
  };

  "sub_scatter_span() of a view trimmed at its last chunk"_test = [&] {
    // Setup
    scatter_array ssa = { first, second, third };
    auto trimmed = ssa.sub_scatter_span({ .count = 7 });

    // Exercise
    auto sub = trimmed.sub_scatter_span({ .offset = 4 });

    // Verify
    expect(that % sub ==
           scatter_span<int>{ std::span(expected).subspan(4, 3) });
  };

  "sub_scatter_span() of every sub-view matches a flat span"_test = [&] {
    // Setup
    // Each layout splits the same elements into chunks differently, including
    // a single chunk and an empty chunk in the middle.
    std::array<int, 9> flat = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    std::span<int> const all(flat);
    std::array<std::span<int>, 1> one_chunk = { all };
    std::array<std::span<int>, 3> three_chunks = { all.first(3),
                                                   all.subspan(3, 2),
                                                   all.subspan(5) };
    std::array<std::span<int>, 4> with_empty = {
      all.first(4), all.subspan(4, 0), all.subspan(4, 1), all.subspan(5)
    };
    std::array<std::span<int>, 9> one_each{};
    for (size_t i = 0; i < flat.size(); i++) {
      one_each[i] = all.subspan(i, 1);
    }
    std::array<std::span<std::span<int> const>, 4> const layouts = {
      one_chunk, three_chunks, with_empty, one_each
    };

    for (auto const layout : layouts) {
      scatter_span<int> whole(layout);
      // Past the end, too, to cover clamping.
      for (size_t offset = 0; offset <= flat.size() + 1; offset++) {
        for (size_t count = 0; count <= flat.size() + 1; count++) {
          // Exercise
          auto outer =
            whole.sub_scatter_span({ .offset = offset, .count = count });
          auto const outer_flat =
            all.subspan(std::min(offset, all.size()))
              .first(
                std::min(count, all.size() - std::min(offset, all.size())));

          // Verify
          expect(that % outer == scatter_span<int>{ outer_flat })
            << "layout chunks:" << layout.size() << "offset:" << offset
            << "count:" << count;

          for (size_t inner_offset = 0; inner_offset <= outer_flat.size() + 1;
               inner_offset++) {
            for (size_t inner_count = 0; inner_count <= outer_flat.size() + 1;
                 inner_count++) {
              // Exercise
              auto inner = outer.sub_scatter_span(
                { .offset = inner_offset, .count = inner_count });
              auto const start = std::min(inner_offset, outer_flat.size());
              auto const inner_flat = outer_flat.subspan(start).first(
                std::min(inner_count, outer_flat.size() - start));

              // Verify
              expect(that % inner == scatter_span<int>{ inner_flat })
                << "layout chunks:" << layout.size() << "offset:" << offset
                << "count:" << count << "inner offset:" << inner_offset
                << "inner count:" << inner_count;
            }
          }
        }
      }
    }
  };

  "sub_scatter_span() single element span that exceeds its bounds"_test = [&] {
    scatter_array ssa = { third };
    // Pick the last three elements of the third array
    auto subssp = ssa.sub_scatter_span({ .offset = 1, .count = 10 });
    scatter_array<int, 1> const expected({ std::span(third).subspan(1, 3) });
    expect(that % subssp == expected);
  };

  "range for scatter_array"_test = [&] {
    scatter_array<int, 3> s_array(first, second, third);
    std::vector<std::span<int const>> exp_span({ first, second, third });
    size_t i = 0;
    for (auto el : s_array) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };

  "range for scatter_span"_test = [&] {
    scatter_array<int, 3> s_array(first, second, third);
    std::vector<std::span<int const>> exp_span({ first, second, third });
    scatter_span<int> s_span(s_array);
    size_t i = 0;
    for (auto el : s_span) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };

  "range for scatter_span (empty)"_test = [&] {
    std::vector<std::span<int const>> exp_span{};
    scatter_span<int> s_span{};
    size_t i = 0;
    for (auto el : s_span) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };
  "for_each_run() splits at whichever chunk boundary comes first"_test = [&] {
    // Setup
    std::array<int, 2> a = { 1, 2 };
    std::array<int, 3> b = { 3, 4, 5 };
    std::array<int, 4> c = { 6, 7, 8, 9 };
    std::array<int, 1> d = { 10 };
    std::vector<std::pair<std::vector<int>, std::vector<int>>> runs;

    // Exercise
    for_each_run(scatter_span<int>{ a, b },
                 scatter_span<int>{ c, d },
                 [&runs](std::span<int> p_first, std::span<int> p_second) {
                   runs.emplace_back(
                     std::vector<int>(p_first.begin(), p_first.end()),
                     std::vector<int>(p_second.begin(), p_second.end()));
                 });

    // Verify
    using run = std::pair<std::vector<int>, std::vector<int>>;
    expect(that % 3uz == runs.size());
    expect(runs[0] == run{ { 1, 2 }, { 6, 7 } });
    expect(runs[1] == run{ { 3, 4 }, { 8, 9 } });
    expect(runs[2] == run{ { 5 }, { 10 } });
  };

  "for_each_run() passes empty spans once one side runs out"_test = [&] {
    // Setup
    std::array<int, 1> a = { 1 };
    std::array<int, 2> b = { 2, 3 };
    std::array<int, 1> c = { 4 };
    std::vector<std::pair<std::size_t, std::size_t>> sizes;

    // Exercise
    for_each_run(scatter_span<int>{ a },
                 scatter_span<int>{ b, c },
                 [&sizes](std::span<int> p_first, std::span<int> p_second) {
                   sizes.emplace_back(p_first.size(), p_second.size());
                 });

    // Verify
    using size_pair = std::pair<std::size_t, std::size_t>;
    expect(that % 3uz == sizes.size());
    expect(sizes[0] == size_pair{ 1, 1 });
    expect(sizes[1] == size_pair{ 0, 1 });
    expect(sizes[2] == size_pair{ 0, 1 });
  };

  "for_each_run() skips empty chunks"_test = [&] {
    // Setup
    std::array<int, 2> a = { 1, 2 };
    std::array<int, 2> b = { 3, 4 };
    std::span<int> empty{};
    int calls = 0;
    int total = 0;

    // Exercise
    for_each_run(scatter_span<int>{ empty, a, empty },
                 scatter_span<int>{ b, empty },
                 [&](std::span<int> p_first, std::span<int> p_second) {
                   calls++;
                   for (auto i = 0uz; i < p_first.size(); i++) {
                     total += p_first[i] * p_second[i];
                   }
                 });

    // Verify
    expect(that % 1 == calls);
    expect(that % ((1 * 3) + (2 * 4)) == total);
  };

  "for_each_run() writes through to the elements"_test = [&] {
    // Setup
    std::array<int, 3> const source = { 1, 2, 3 };
    std::array<int, 1> head{};
    std::array<int, 2> tail{};

    // Exercise
    for_each_run(scatter_span<int const>{ source },
                 scatter_span<int>{ head, tail },
                 [](std::span<int const> p_from, std::span<int> p_to) {
                   std::ranges::copy(p_from, p_to.begin());
                 });

    // Verify
    expect(that % 1 == head[0]);
    expect(tail == std::array<int, 2>{ 2, 3 });
  };

  "for_each_run() of two empty scatter spans calls nothing"_test = [&] {
    // Setup
    int calls = 0;

    // Exercise
    for_each_run(scatter_span<int>{},
                 scatter_span<int>{},
                 [&calls](std::span<int>, std::span<int>) { calls++; });

    // Verify
    expect(that % 0 == calls);
  };
};
}  // namespace

int main()
{
  basic_scatter_span_tests();
}
