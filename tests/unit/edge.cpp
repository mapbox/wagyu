#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("test edge initialization - same two points") {
    mapbox::geometry::point<T> p1 = { 100, 10 };
    mapbox::geometry::point<T> p2 = { 100, 10 };
    edge<T> e1(p1, p2);

    CHECK(e1.bot.x == 100);
    CHECK(e1.bot.y == 10);
    CHECK(e1.top.x == 100);
    CHECK(e1.top.y == 10);
    CHECK(std::isinf(e1.dx));
}

TEST_CASE("test edge initialization - different type") {
    mapbox::geometry::point<std::int32_t> p1 = { 100, 10 };
    mapbox::geometry::point<std::int32_t> p2 = { 10, 15 };
    edge<std::int64_t> e1(p1, p2);

    CHECK(e1.bot.x == 10);
    CHECK(e1.bot.y == 15);
    CHECK(e1.top.x == 100);
    CHECK(e1.top.y == 10);
    CHECK(e1.dx == Approx(-18.0));
}

TEST_CASE("test edge initialization - horizontal segment") {
    mapbox::geometry::point<T> p1 = { 10, 10 };
    mapbox::geometry::point<T> p2 = { 100, 10 };
    edge<T> e1(p1, p2);
    edge<T> e2(p2, p1);

    CHECK(e1.bot.x == 10);
    CHECK(e1.bot.y == 10);
    CHECK(e1.top.x == 100);
    CHECK(e1.top.y == 10);
    CHECK(std::isinf(e1.dx));

    CHECK(e2.bot.x == 100);
    CHECK(e2.bot.y == 10);
    CHECK(e2.top.x == 10);
    CHECK(e2.top.y == 10);
    CHECK(std::isinf(e2.dx));
}

TEST_CASE("test edge initialization - vertical segment") {
    mapbox::geometry::point<T> p1 = { 10, 10 };
    mapbox::geometry::point<T> p2 = { 10, 100 };
    edge<T> e1(p1, p2);
    edge<T> e2(p2, p1);

    CHECK(e1.bot.x == 10);
    CHECK(e1.bot.y == 100);
    CHECK(e1.top.x == 10);
    CHECK(e1.top.y == 10);
    CHECK(e1.dx == Approx(0.0));

    CHECK(e2.bot.x == 10);
    CHECK(e2.bot.y == 100);
    CHECK(e2.top.x == 10);
    CHECK(e2.top.y == 10);
    CHECK(e2.dx == Approx(0.0));
}
