#include <catch2/catch_test_macros.hpp>

import woof.jobs;

TEST_CASE("JobSpec.getId()", "[jobspec]") {
  const woof::JobSpec base{"command", "echo Hello, World!"};

  SECTION("is different on different type")
  {
    woof::JobSpec other = base;
    other.type = "other";
    REQUIRE(base.getId() != other.getId());
  }

  SECTION("is different on different run specification")
  {
    woof::JobSpec other = base;
    other.runSpec = "echo Something else";
    REQUIRE(base.getId() != other.getId());
  }
}
