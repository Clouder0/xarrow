#include "aligned_vector.hpp"
#include "doctest/doctest.h"
#include <cstdint>
#include <memory>

using namespace xarrow;

TEST_CASE("aligned vector init") {
  AlignedVector<int8_t, 64> test2;
  int8_t *data;
  {
    AlignedVector<int8_t, 64> test;
    REQUIRE(test.size() == 0);
    CHECK(test.capacity() == 4);

    test.emplace_back(1);
    test.emplace_back(2);
    test.emplace_back(3);
    test.emplace_back(4);

    CHECK(test.at(0) == 1);
    CHECK(test.at(1) == 2);
    CHECK(test.at(2) == 3);
    CHECK(test.at(3) == 4);

    test.emplace_back(5);
    CHECK(test.at(4) == 5);
    CHECK(test.size() == 5);
    CHECK(test.capacity() > 5);
    data = test.data();
    test2 = std::move(test);
  }
  CHECK(test2.size() == 5);
  CHECK(test2.data() == data);
  CHECK(reinterpret_cast<std::uintptr_t>(test2.data()) % 64 == 0);
}

TEST_CASE("aligned vector resize") {
  AlignedVector<int, 16> vec;
  vec.resize(5, 10);
  CHECK(vec.size() == 5);
  CHECK(vec[0] == 10);
  CHECK(vec[4] == 10);

  vec.resize(2);
  CHECK(vec.size() == 2);
  CHECK(vec[0] == 10);
  CHECK(vec[1] == 10);

  vec.resize(10, 20);
  CHECK(vec.size() == 10);
  CHECK(vec[0] == 10); // First two elements remain unchanged.
  CHECK(vec[1] == 10);
  CHECK(vec[2] == 20);
  CHECK(vec[9] == 20);
}

TEST_CASE("aligned vector push_back/pop_back") {
  AlignedVector<int, 16> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  CHECK(vec.size() == 3);
  CHECK(vec[0] == 1);
  CHECK(vec[1] == 2);
  CHECK(vec[2] == 3);

  vec.pop_back();
  CHECK(vec.size() == 2);
  CHECK(vec[0] == 1);
  CHECK(vec[1] == 2);

  vec.pop_back();
  vec.pop_back(); // Pop from empty vector should not crash.
  CHECK(vec.size() == 0);
}

TEST_CASE("aligned vector assign") {
  AlignedVector<int, 16> vec1;
  vec1.assign(5, 1);
  CHECK(vec1.size() == 5);
  std::fill(vec1.begin(), vec1.end(), 1);

  AlignedVector<int, 16> vec2;
  vec2 = vec1; // copy assignment
  CHECK(vec2.size() == 5);
  CHECK(vec2 == vec1);

  AlignedVector<int, 16> vec3;
  vec3.assign(3, 2);
  vec3 = std::move(vec1); // move assignment
  CHECK(vec3.size() == 5);
  CHECK(vec3[0] == 1);
}

TEST_CASE("aligned vector iterators") {
  AlignedVector<int, 16> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);

  int sum = 0;
  for (int i : vec) {
    sum += i;
  }
  CHECK(sum == 6);

  sum = 0;
  for (int const &it : vec) {
    sum += it;
  }
  CHECK(sum == 6);

  sum = 0;
  for (auto it = vec.rbegin(); it != vec.rend(); ++it) {
    sum += *it;
  }
  CHECK(sum == 6);
}

TEST_CASE("aligned vector exception handling") {
  AlignedVector<int, 16> vec;
  CHECK_THROWS_AS(vec.at(0), std::out_of_range);
  vec.push_back(1);
  CHECK_NOTHROW(vec.at(0));
  CHECK_THROWS_AS(vec.at(1), std::out_of_range);
}

TEST_CASE("aligned vector comparison operators") {
  AlignedVector<int, 16> vec1, vec2, vec3;
  vec1.push_back(1);
  vec1.push_back(2);
  vec2.push_back(1);
  vec2.push_back(2);
  vec3.push_back(1);
  vec3.push_back(3);

  CHECK(vec1 == vec2);
  CHECK(vec1 != vec3);
  CHECK(vec1 < vec3);
  CHECK(vec3 > vec1);
  CHECK(vec1 <= vec2);
  CHECK(vec1 >= vec2);
}

TEST_CASE("aligned vector memory safety - large allocations") {
  // Test with a large number of elements to stress memory allocation
  size_t large_size = 10000;
  AlignedVector<int, 64> vec;
  vec.reserve(large_size);
  for (size_t i = 0; i < large_size; ++i) {
    vec.emplace_back(i);
  }
  CHECK(vec.size() == large_size);
  for (size_t i = 0; i < large_size; ++i) {
    CHECK(vec[i] == i);
  }
  vec.clear(); // Ensure clear doesn't cause issues with large allocations.
  CHECK(vec.size() == 0);
}

TEST_CASE("aligned vector move semantics") {
  AlignedVector<int, 16> vec1;
  vec1.push_back(1);
  vec1.push_back(2);
  vec1.push_back(3);

  AlignedVector<int, 16> vec2 = std::move(vec1); // Move constructor

  CHECK(vec2.size() == 3);
  CHECK(vec2[0] == 1);
  CHECK(vec2[1] == 2);
  CHECK(vec2[2] == 3);
  CHECK(vec1.size() == 0); // vec1 should be empty after move

  AlignedVector<int, 16> vec3;
  vec3.push_back(4);
  vec3.push_back(5);
  vec3 = std::move(vec2); // Move assignment

  CHECK(vec3.size() == 3);
  CHECK(vec3[0] == 1);
  CHECK(vec3[1] == 2);
  CHECK(vec3[2] == 3);
  CHECK(vec2.size() == 0); // vec2 should be empty after move

  // Test move with different alignment
  AlignedVector<int64_t, 64> vec4;
  vec4.push_back(100);
  AlignedVector<int64_t, 64> vec5 = std::move(vec4);
  CHECK(vec5.size() == 1);
  CHECK(vec5[0] == 100);
  CHECK(vec4.size() == 0);
}

TEST_CASE("aligned vector self move assignment") {
  // Test self-move assignment
  AlignedVector<int, 16> vec;
  vec.push_back(1);
  vec = std::move(vec);
  CHECK(vec.size() == 1);
  CHECK(vec[0] == 1);
}

TEST_CASE("aligned vector shrink_to_fit") {
  AlignedVector<int, 16> vec;
  vec.reserve(100);
  vec.push_back(1);
  CHECK(vec.capacity() == 100);
  vec.shrink_to_fit();
  CHECK(vec.capacity() ==
        1); // or a reasonable small capacity, depending on implementation
}

TEST_CASE("aligned vector reallocation with zero capacity") {
  AlignedVector<int, 16> vec;
  vec.push_back(1);
  vec.push_back(2);
  vec.reallocate(0); // should clear the vector correctly
  CHECK(vec.size() == 0);
  CHECK(vec.capacity() == 0); // or a small default capacity
}

TEST_CASE("aligned vector stress test - many push_back and pop_back") {
  AlignedVector<int, 16> vec;
  for (int i = 0; i < 10000; ++i) {
    vec.push_back(i);
    if (i % 100 == 0) {
      vec.pop_back();
    }
  }
  CHECK(vec.size() > 0); // Check for potential memory corruption after many
                         // push_back/pop_back.
}
