#include "hey.hpp"
#include "common.hpp"

auto hey_test() -> int { return 1; }

#ifdef TEST
TEST_CASE("hey test should return 1") { CHECK(hey_test() == 1); }
#endif
