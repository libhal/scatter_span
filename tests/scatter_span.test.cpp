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

#include <array>
#include <boost/ut.hpp>
#include <cstddef>
#include <cstdint>
#include <print>
#include <vector>
import scatter_span;

namespace mem {

}  // namespace mem

namespace mem {

// Exclusively exists to do comparisons, there isn't really a need for a
// comparison between scatter spans.
// if discovered
template<typename T>
bool scatter_span_eq(mem::scatter_span<T> const& lhs,
                     mem::scatter_span<T> const& rhs)
{
  auto len = lhs.length();
  if (len != rhs.length()) {
    return false;
  }

  std::vector<T> lhs_vec;
  std::vector<T> rhs_vec;

  for (auto s : lhs) {
    lhs_vec.append_range(s);
  }

  for (auto s : rhs) {
    rhs_vec.append_range(s);
  }

  for (size_t i = 0; i < lhs_vec.size(); i++) {
    if (lhs_vec[i] != rhs_vec[i]) {
      return false;
    }
  }

  return true;
}

template<typename T>
void print_scatter_span(scatter_span<T> const& p_ssp)
{
  std::print("[");
  for (auto s : p_ssp) {
    std::print("{}, ", s);
  }
  std::println("]");
}

template<typename T>
void print_scatter_span_addrs(scatter_span<T> const& p_ssp)
{
  size_t count = 0;
  for (auto s : p_ssp) {
    std::println("{}: {:#x}, ", count, reinterpret_cast<uintptr_t>(&s[0]));
    count++;
  }
  std::println("");
}

}  // namespace mem

boost::ut::suite<"scatter_span"> basic_scatter_span_tests = [] {
  using namespace boost::ut;
  using namespace mem;

  std::array<int, 3> first = { 1, 2, 3 };
  std::array<int, 2> second{ 4, 5 };
  std::array<int, 4> third = { 6, 7, 8, 9 };
  std::array<int, 9> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };

  "ctor_and_len"_test = [&] {
    std::array<int, 3> first = { 1, 2, 3 };
    std::array<int, 2> second{ 4, 5 };
    std::array<int, 4> third = { 6, 7, 8, 9 };

    mem::scatter_array<int, 3> ssp(first, second, third);
    mem::print_scatter_span(ssp);
    mem::print_scatter_span_addrs(ssp);
    expect(that % ssp.length() == 9);

    mem::scatter_array<int, 1> expected_ssp({ expected });
    expect(that % scatter_span_eq(ssp, expected_ssp));
  };

  "api_lifetime"_test = [&] {
    auto mock_api = [&first, &second, &third](scatter_span<int> p_ssp) {
      std::array<int, 9> expected = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
      mem::scatter_array<int, 1> expected_ssp(expected);
      expect(that % scatter_span_eq(expected_ssp, p_ssp));

      auto ssp_it = p_ssp.begin();
      expect(that % first.data() == ssp_it->data());
      ssp_it++;
      expect(that % second.data() == ssp_it->data());
      ssp_it++;
      expect(that % third.data() == ssp_it->data());
    };

    mock_api({ first, second, third });
  };

  "subscatterspan"_test = [&] {
    auto ssa = scatter_array(first, second, third);

    auto subssp = ssa.sub_scatter_span({ .offset = 0, .count = 5 });

    expect(that %
           scatter_span_eq(subssp, { std::span(expected).subspan(0, 5) }));

    auto unevenssp = ssa.sub_scatter_span({ .offset = 0, .count = 6 });

    expect(that %
           scatter_span_eq(unevenssp, { std::span(expected).subspan(0, 6) }));
  };
};

int main()
{
  return static_cast<int>(boost::ut::cfg<>.run());
}
