#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "hey.hpp"

TEST_CASE("simple test") { CHECK(hey_test() == 1); }
