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
bool operator==(mem::scatter_span<T> const& lhs,
                mem::scatter_span<T> const& rhs)
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
    mem::scatter_span<int> empty_scatter_span{};
    expect(that % empty_scatter_span.length() == 0);
  };

  "constructor & length"_test = [&] {
    mem::scatter_array<int, 3> ssp(first, second, third);
    expect(that % ssp.length() == 9);

    mem::scatter_array<int, 1> expected_ssp({ expected });
    expect(that % ssp == expected_ssp);
  };

  "api_lifetime"_test = [&] {
    auto mock_api = [&first, &second, &third](scatter_span<int> p_ssp) {
      std::array<int, 9> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
      mem::scatter_array<int, 1> expected_ssp(expected);
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

  ".sub_scatter_span() empty span"_test = [&] {
    scatter_span<int> ssa{};

    auto subssp = ssa.sub_scatter_span({ .offset = 0, .count = 5 });

    expect(that % 0uz == ssa.length());
    expect(that % subssp == scatter_span<int>{});
  };

  ".sub_scatter_span() single element span"_test = [&] {
    scatter_array<int, 1> ssa({ third });
    // Pick the middle two elements of the third array
    auto subssp = ssa.sub_scatter_span({ .offset = 1, .count = 2 });
    scatter_array<int, 1> const expected({ std::span(third).subspan(1, 2) });
    expect(that % subssp == expected);
  };

  "range for scatter_array"_test = [&] {
    mem::scatter_array<int, 3> s_array(first, second, third);
    std::vector<std::span<int const>> exp_span({ first, second, third });
    size_t i = 0;
    for (auto el : s_array) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };

  "range for scatter_span"_test = [&] {
    mem::scatter_array<int, 3> s_array(first, second, third);
    std::vector<std::span<int const>> exp_span({ first, second, third });
    mem::scatter_span<int> s_span(s_array);
    size_t i = 0;
    for (auto el : s_span) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };

  "range for scatter_span (empty)"_test = [&] {
    std::vector<std::span<int const>> exp_span{};
    mem::scatter_span<int> s_span{};
    size_t i = 0;
    for (auto el : s_span) {
      expect(that % std::ranges::equal(exp_span[i], el));
      i++;
    }
  };
};
}  // namespace

int main()
{
  basic_scatter_span_tests();
}
