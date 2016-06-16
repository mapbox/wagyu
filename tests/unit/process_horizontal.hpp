#include "catch.hpp"

#include <mapbox/geometry/wagyu/process_horizontal.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("get_horizontal_direction finds edge direction") {
    mapbox::geometry::point<T> p1 = { 0, 5 };
    mapbox::geometry::point<T> p2 = { 0, 7 };
    edge<T> e1(p1, p2, polygon_type_subject);

    T left;
    T right;
    horizontal_direction dir;

    process_horizontal(&e1, dir, left, right);

    CHECK(dir == left_to_right);
    CHECK(left == p1.x);
    CHECK(right == p2.x);

    // flip horizontal direction
    edge<T> e2(p2, p1, polygon_type_subject);

    process_horizontal(&e2, dir, left, right);

    CHECK(dir == right_to_left);
    CHECK(left == p2.x);
    CHECK(right == p1.x);
};