#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

import woof.jobs;

TEST_CASE("JobId encoding")
{
    const woof::JobId id{1234567890};
    REQUIRE(woof::JobId{id.toString()} == id);
}

TEST_CASE("JobSpec.getId()", "[jobspec]")
{
    const woof::JobSpec base{
        "command",
        "echo Hello, World!",
        {{"dep1", woof::JobId{34567}}, {"dep2", woof::JobId{45678}}}
    };

    SECTION("is different on different type") {
        woof::JobSpec other = base;
        other.type = "other";
        REQUIRE(base.getId() != other.getId());
    }

    SECTION("is different on different run specification") {
        woof::JobSpec other = base;
        other.runSpec = "echo Something else";
        REQUIRE(base.getId() != other.getId());
    }

    SECTION("is different on more dependencies") {
        woof::JobSpec other = base;
        other.dependencies.emplace("dep3", woof::JobId{98765});
        REQUIRE(base.getId() != other.getId());
    }

    SECTION("is different on dependencies with different IDs") {
        woof::JobSpec other = base;
        other.dependencies.erase("dep1");
        other.dependencies.emplace("dep1", woof::JobId{100000});
        REQUIRE(base.getId() != other.getId());
    }

    SECTION("is different on dependencies with different names") {
        woof::JobSpec other = base;
        other.dependencies.emplace("dep3", other.dependencies.at("dep1"));
        other.dependencies.erase("dep1");
        REQUIRE(base.getId() != other.getId());
    }
}
