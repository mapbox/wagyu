#include "catch.hpp"

#include <mapbox/geometry/wagyu/bound.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/intersect_point.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("test intersection of points") {
    mapbox::geometry::point<T> p1 = { -1, -2 };
    mapbox::geometry::point<T> p2 = { 9, 5 };
    mapbox::geometry::point<T> p3 = { 0, 0 };
    mapbox::geometry::point<T> p4 = { 13, 6 };

    // Initialize result points
    mapbox::geometry::point<T> r1 = { 0, 0 };

    bound<T> b1;
    b1.edges.emplace_back(p1, p2);
    edge<T>& e1 = b1.edges.back();
    bound<T> b2;
    b2.edges.emplace_back(p2, p3);
    edge<T>& e2 = b2.edges.back();
    bound<T> b3;
    b3.edges.emplace_back(p3, p4);
    edge<T>& e3 = b3.edges.back();

    // Initialization would be as such
    b1.curr = e1.top;
    b2.curr = e2.bot;
    b3.curr = e3.top;
    b1.current_edge = b1.edges.begin();
    b2.current_edge = b2.edges.begin();
    b3.current_edge = b3.edges.begin();

    CHECK(e1.bot.x == 9);
    CHECK(e1.bot.y == 5);
    CHECK(b1.curr.x == -1);
    CHECK(b1.curr.y == -2);
    CHECK(e1.top.x == -1);
    CHECK(e1.top.y == -2);
    CHECK(e1.dx == Approx(1.4285714286));

    CHECK(e2.bot.x == 9);
    CHECK(e2.bot.y == 5);
    CHECK(b2.curr.x == 9);
    CHECK(b2.curr.y == 5);
    CHECK(e2.top.x == 0);
    CHECK(e2.top.y == 0);
    CHECK(e2.dx == Approx(1.8));

    CHECK(e3.bot.x == 13);
    CHECK(e3.bot.y == 6);
    CHECK(b3.curr.x == 0);
    CHECK(b3.curr.y == 0);
    CHECK(e3.top.x == 0);
    CHECK(e3.top.y == 0);
    CHECK(e3.dx == Approx(2.1666666667));

    T top_y = -2;
    // Scanbeam would start at -2 and add e1
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    CHECK(b1.curr.x == -1);
    CHECK(b1.curr.y == -2);
    // Next scanbeam would reach 0 and add e2 and e3
    top_y = 0;
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    b2.curr.y = top_y;
    b2.curr.x = get_current_x(e2, top_y);
    b3.curr.y = top_y;
    b3.curr.x = get_current_x(e3, top_y);
    CHECK(b1.curr.x == 2);
    CHECK(b1.curr.y == 0);
    CHECK(b2.curr.x == 0);
    CHECK(b2.curr.y == 0);
    CHECK(b3.curr.x == 0);
    CHECK(b3.curr.y == 0);
    // The active edge list would be sorted by increasing x
    // so would be e3, e2, e1
    // Show that intersection will not be run due to no endpoint intersections
    CHECK_FALSE(b3.curr.x > b2.curr.x); // No intersection of points!
    CHECK_FALSE(b2.curr.x > b1.curr.x); // No intersection of points!
    // Next scanbeam would reach 5
    top_y = 5;
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    b2.curr.y = top_y;
    b2.curr.x = get_current_x(e2, top_y);
    b3.curr.y = top_y;
    b3.curr.x = get_current_x(e3, top_y);
    CHECK(b1.curr.x == 9);
    CHECK(b1.curr.y == 5);
    CHECK(b2.curr.x == 9);
    CHECK(b2.curr.y == 5);
    CHECK(b3.curr.x == 11);
    CHECK(b3.curr.y == 5);
    // The active edge list would be e2, e1, e3
    CHECK_FALSE(b1.curr.x > b2.curr.x); // No intersection of points
    CHECK(b3.curr.x > b1.curr.x);       // Intersection!
    intersection_point(b1, b3, r1);

    CHECK(r1.x == 5);
    CHECK(r1.y == 2);
}

TEST_CASE("test intersection of points - switch axis values") {
    mapbox::geometry::point<T> p1 = { -2, -1 };
    mapbox::geometry::point<T> p2 = { 5, 9 };
    mapbox::geometry::point<T> p3 = { 0, 0 };
    mapbox::geometry::point<T> p4 = { 6, 13 };

    // Initialize result points
    mapbox::geometry::point<T> r1 = { 0, 0 };

    bound<T> b1;
    b1.edges.emplace_back(p1, p2);
    edge<T>& e1 = b1.edges.back();
    bound<T> b2;
    b2.edges.emplace_back(p2, p3);
    edge<T>& e2 = b2.edges.back();
    bound<T> b3;
    b3.edges.emplace_back(p3, p4);
    edge<T>& e3 = b3.edges.back();

    // Initialization would be as such
    b1.curr = e1.top;
    b2.curr = e2.bot;
    b3.curr = e3.top;
    b1.current_edge = b1.edges.begin();
    b2.current_edge = b2.edges.begin();
    b3.current_edge = b3.edges.begin();

    CHECK(e1.bot.x == 5);
    CHECK(e1.bot.y == 9);
    CHECK(b1.curr.x == -2);
    CHECK(b1.curr.y == -1);
    CHECK(e1.top.x == -2);
    CHECK(e1.top.y == -1);
    CHECK(e1.dx == Approx(0.7));

    CHECK(e2.bot.x == 5);
    CHECK(e2.bot.y == 9);
    CHECK(b2.curr.x == 5);
    CHECK(b2.curr.y == 9);
    CHECK(e2.top.x == 0);
    CHECK(e2.top.y == 0);
    CHECK(e2.dx == Approx(0.5555555556));

    CHECK(e3.bot.x == 6);
    CHECK(e3.bot.y == 13);
    CHECK(b3.curr.x == 0);
    CHECK(b3.curr.y == 0);
    CHECK(e3.top.x == 0);
    CHECK(e3.top.y == 0);
    CHECK(e3.dx == Approx(0.4615384615));

    T top_y = -1;
    // Scanbeam would start at -1 and add e1
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    CHECK(b1.curr.x == -2);
    CHECK(b1.curr.y == -1);
    // Next scanbeam would reach 0 and add e2 and e3
    top_y = 0;
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    b2.curr.y = top_y;
    b2.curr.x = get_current_x(e2, top_y);
    b3.curr.y = top_y;
    b3.curr.x = get_current_x(e3, top_y);
    CHECK(b1.curr.x == -1);
    CHECK(b1.curr.y == 0);
    CHECK(b2.curr.x == 0);
    CHECK(b2.curr.y == 0);
    CHECK(b3.curr.x == 0);
    CHECK(b3.curr.y == 0);
    // The active edge list would be sorted by increasing x
    // so would be e1, e2, e3
    // Show that intersection will not be run due to no intersections at origin
    CHECK_FALSE(b1.curr.x > b2.curr.x); // No intersection of points!
    CHECK_FALSE(b1.curr.x > b3.curr.x); // No intersection of points!
    // Next scanbeam would reach 9
    top_y = 9;
    b1.curr.y = top_y;
    b1.curr.x = get_current_x(e1, top_y);
    b2.curr.y = top_y;
    b2.curr.x = get_current_x(e2, top_y);
    b3.curr.y = top_y;
    b3.curr.x = get_current_x(e3, top_y);
    CHECK(b1.curr.x == 5);
    CHECK(b1.curr.y == 9);
    CHECK(b2.curr.x == 5);
    CHECK(b2.curr.y == 9);
    CHECK(b3.curr.x == 4);
    CHECK(b3.curr.y == 9);
    // The active edge list would be e1, e2, e3
    CHECK_FALSE(b1.curr.x > b2.curr.x); // No intersection of points
    CHECK(b1.curr.x > b3.curr.x);       // Intersection!
    intersection_point(b1, b3, r1);

    CHECK(r1.x == 3);
    CHECK(r1.y == 5);
}
