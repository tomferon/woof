#include <functional>
#include <boost/beast/http/verb.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

import woof.http_server;

namespace http = boost::beast::http;

using namespace woof::http_server;
using namespace std::literals;

TEST_CASE("Matching static route", "[http_routing]")
{
    constexpr std::span<std::string_view> noFragments{};

    SECTION("GET /") {
        const auto matcher = method<int>(http::verb::get, std::function{[]() { return 42; }});
        REQUIRE(matcher->handle(http::verb::get, noFragments) == 42);
    }

    SECTION("GET /a/b/c") {
        const auto matcher = "a" / ("b" / ("c" / method<int>(http::verb::get, std::function{[]() { return 42; }})));
        std::string_view fragments[]{"a"sv, "b"sv, "c"sv};
        REQUIRE(matcher->handle(http::verb::get, fragments) == 42);
    }

    SECTION("POST /") {
        const auto matcher = method<int>(http::verb::post, std::function{[]() { return 42; }});
        REQUIRE(matcher->handle(http::verb::post, noFragments) == 42);
    }
}

// FIXME: Ideally, it should throw MethodNotAllowed so we can return a 405.
TEST_CASE("Wrong method", "[http_routing]")
{
    constexpr std::span<std::string_view> noFragments{};

    SECTION("GET /") {
        const auto matcher = method<int>(http::verb::get, std::function{[]() { return 42; }});
        REQUIRE_THROWS_AS(matcher->handle(http::verb::post, noFragments), RouteDoesntMatch);
    }

    SECTION("GET /a/b/c") {
        const auto matcher = "a" / ("b" / ("c" /
            method<int>(http::verb::get, std::function{[]() { return 42; }})));
        std::string_view fragments[]{"a"sv, "b"sv, "c"sv};
        REQUIRE_THROWS_AS(matcher->handle(http::verb::post, fragments), RouteDoesntMatch);
    }

    SECTION("POST /") {
        const auto matcher = method<int>(http::verb::post, std::function{[]() { return 42; }});
        REQUIRE_THROWS_AS(matcher->handle(http::verb::get, noFragments), RouteDoesntMatch);
    }
}

TEST_CASE("Partial matches", "[http_routing]")
{
    SECTION("Route has longer path") {
        const auto matcher = "a" / ("b" / method<int>(http::verb::get, std::function{[]() { return 42; }}));
        std::string_view fragments[]{"a"sv};
        REQUIRE_THROWS_AS(matcher->handle(http::verb::get, fragments), RouteDoesntMatch);
    }

    SECTION("Route has shorter path") {
        const auto matcher = "a" / method<int>(http::verb::get, std::function{[]() { return 42; }});
        std::string_view fragments[]{"a"sv, "b"sv};
        REQUIRE_THROWS_AS(matcher->handle(http::verb::get, fragments), RouteDoesntMatch);
    }
}

TEST_CASE("Capturing route fragments", "[http_routing]")
{
    SECTION("GET /a/:some_string/c") {
        const auto matcher = "a" /
                capture<std::string_view, std::string_view>(std::function{[](std::string_view str) {
                    return "c" / method<std::string_view>(http::verb::get, std::function{[str]() { return str; }});
                }});
        std::string_view fragments[]{"a"sv, "success"sv, "c"sv};
        REQUIRE(matcher->handle(http::verb::get, fragments) == "success");
    }

    SECTION("POST /a/:some_integer/c") {
        const auto matcher = "a" /
                capture<int, int>(std::function{[](int x) {
                    return "c" / method<int>(http::verb::get, std::function{[x]() { return x; }});
                }});
        std::string_view fragments[]{"a"sv, "42"sv, "c"sv};
        REQUIRE(matcher->handle(http::verb::get, fragments) == 42);
    }

    WHEN("invalid integer") {
        SECTION("POST /a/:some_integer/c") {
            const auto matcher = "a" /
                    capture<int, int>(std::function{[](int x) {
                        return "c" / method<int>(http::verb::get, std::function{[x]() { return x; }});
                    }});
            std::string_view fragments[]{"a"sv, "b"sv, "c"sv};
            REQUIRE_THROWS_AS(matcher->handle(http::verb::get, fragments), RouteDoesntMatch);
        }
    }
}

TEST_CASE("Combining routes", "[http_routing]")
{
    SECTION("GET /a/b/c and POST /a/b/c") {
        const auto matcher = "a" / ("b" / ("c" / routes({
            method<int>(http::verb::get, std::function{[]() { return 42; }}),
            method<int>(http::verb::post, std::function{[]() { return 43; }}),
        })));
        std::string_view fragments[]{"a"sv, "b"sv, "c"sv};
        REQUIRE(matcher->handle(http::verb::get, fragments) == 42);
        REQUIRE(matcher->handle(http::verb::post, fragments) == 43);
    }

    SECTION("First has priority when ambiguous") {
        const auto matcher = "a" / routes({
            capture<std::string_view, std::string_view>(std::function{[](std::string_view str) {
                return method<std::string_view>(http::verb::get, std::function{[str]() { return str; }});
            }}),
            "b" / method<std::string_view>(http::verb::post, std::function{[]() { return "wrong"sv; }}),
        });
        std::string_view fragments[]{"a"sv, "b"sv};
        REQUIRE(matcher->handle(http::verb::get, fragments) == "b"sv);
    }

    SECTION("Partial match") {
        const auto matcher = "a" / routes({
            method<int>(http::verb::get, std::function{[]() { return 1; }}),
            "b" / method<int>(http::verb::post, std::function{[]() { return 2; }}),
        });
        std::string_view fragments[]{"a"sv, "b"sv};
        REQUIRE(matcher->handle(http::verb::post, fragments) == 2);
    }
}
