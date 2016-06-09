#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/intersect_point.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

TEST_CASE("test intersection of points")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { -1, -2 };
    mapbox::geometry::point<std::int64_t> p2 = { 9, 5 };
    mapbox::geometry::point<std::int64_t> p3 = { 0, 0 };
    mapbox::geometry::point<std::int64_t> p4 = { 13, 6 };
    
    // Initialize result points
    mapbox::geometry::point<std::int64_t> r1 = { 0, 0 };
    
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);
    edge<std::int64_t> e2(p2, p3, polygon_type_subject);
    edge<std::int64_t> e3(p3, p4, polygon_type_subject);

    CHECK(e1.Bot.x == 9);
    CHECK(e1.Bot.y == 5);
    CHECK(e1.Curr.x == -1);
    CHECK(e1.Curr.y == -2);
    CHECK(e1.Top.x == -1);
    CHECK(e1.Top.y == -2);
    CHECK(e1.Dx == Approx(1.4285714286));
    
    CHECK(e2.Bot.x == 9);
    CHECK(e2.Bot.y == 5);
    CHECK(e2.Curr.x == 9);
    CHECK(e2.Curr.y == 5);
    CHECK(e2.Top.x == 0);
    CHECK(e2.Top.y == 0);
    CHECK(e2.Dx == Approx(1.8));
    
    CHECK(e3.Bot.x == 13);
    CHECK(e3.Bot.y == 6);
    CHECK(e3.Curr.x == 0);
    CHECK(e3.Curr.y == 0);
    CHECK(e3.Top.x == 0);
    CHECK(e3.Top.y == 0);
    CHECK(e3.Dx == Approx(2.1666666667));

    std::int64_t top_y = -2;
    // Scanbeam would start at -2 and add e1
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    CHECK(e1.Curr.x == -1);
    CHECK(e1.Curr.y == -2);
    // Next scanbeam would reach 0 and add e2 and e3
    top_y = 0;
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    e2.Curr.y = top_y;
    e2.Curr.x = TopX(e2, top_y);
    e3.Curr.y = top_y;
    e3.Curr.x = TopX(e3, top_y);
    CHECK(e1.Curr.x == 2);
    CHECK(e1.Curr.y == 0);
    CHECK(e2.Curr.x == 0);
    CHECK(e2.Curr.y == 0);
    CHECK(e3.Curr.x == 0);
    CHECK(e3.Curr.y == 0);
    // The active edge list would be sorted by increasing x
    // so would be e3, e2, e1
    // Show that intersection will not be run due to no endpoint intersections
    CHECK_FALSE(e3.Curr.x > e2.Curr.x); // No intersection of points!
    CHECK_FALSE(e2.Curr.x > e1.Curr.x); // No intersection of points!
    // Next scanbeam would reach 5
    top_y = 5;
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    e2.Curr.y = top_y;
    e2.Curr.x = TopX(e2, top_y);
    e3.Curr.y = top_y;
    e3.Curr.x = TopX(e3, top_y);
    CHECK(e1.Curr.x == 9);
    CHECK(e1.Curr.y == 5);
    CHECK(e2.Curr.x == 9);
    CHECK(e2.Curr.y == 5);
    CHECK(e3.Curr.x == 11);
    CHECK(e3.Curr.y == 5);
    // The active edge list would be e2, e1, e3
    CHECK_FALSE(e1.Curr.x > e2.Curr.x); // No intersection of points
    CHECK(e3.Curr.x > e1.Curr.x); // Intersection!
    intersection_point(e1, e3, r1);

    CHECK(r1.x == 7);
    CHECK(r1.y == 3);
}

TEST_CASE("test intersection of points - switch axis values")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { -2, -1 };
    mapbox::geometry::point<std::int64_t> p2 = { 5, 9 };
    mapbox::geometry::point<std::int64_t> p3 = { 0, 0 };
    mapbox::geometry::point<std::int64_t> p4 = { 6, 13 };
    
    // Initialize result points
    mapbox::geometry::point<std::int64_t> r1 = { 0, 0 };
    
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);
    edge<std::int64_t> e2(p2, p3, polygon_type_subject);
    edge<std::int64_t> e3(p3, p4, polygon_type_subject);

    CHECK(e1.Bot.x == 5);
    CHECK(e1.Bot.y == 9);
    CHECK(e1.Curr.x == -2);
    CHECK(e1.Curr.y == -1);
    CHECK(e1.Top.x == -2);
    CHECK(e1.Top.y == -1);
    CHECK(e1.Dx == Approx(0.7));
    
    CHECK(e2.Bot.x == 5);
    CHECK(e2.Bot.y == 9);
    CHECK(e2.Curr.x == 5);
    CHECK(e2.Curr.y == 9);
    CHECK(e2.Top.x == 0);
    CHECK(e2.Top.y == 0);
    CHECK(e2.Dx == Approx(0.5555555556));
    
    CHECK(e3.Bot.x == 6);
    CHECK(e3.Bot.y == 13);
    CHECK(e3.Curr.x == 0);
    CHECK(e3.Curr.y == 0);
    CHECK(e3.Top.x == 0);
    CHECK(e3.Top.y == 0);
    CHECK(e3.Dx == Approx(0.4615384615));

    std::int64_t top_y = -1;
    // Scanbeam would start at -1 and add e1
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    CHECK(e1.Curr.x == -2);
    CHECK(e1.Curr.y == -1);
    // Next scanbeam would reach 0 and add e2 and e3
    top_y = 0;
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    e2.Curr.y = top_y;
    e2.Curr.x = TopX(e2, top_y);
    e3.Curr.y = top_y;
    e3.Curr.x = TopX(e3, top_y);
    CHECK(e1.Curr.x == -1);
    CHECK(e1.Curr.y == 0);
    CHECK(e2.Curr.x == 0);
    CHECK(e2.Curr.y == 0);
    CHECK(e3.Curr.x == 0);
    CHECK(e3.Curr.y == 0);
    // The active edge list would be sorted by increasing x
    // so would be e1, e2, e3
    // Show that intersection will not be run due to no intersections at origin
    CHECK_FALSE(e1.Curr.x > e2.Curr.x); // No intersection of points!
    CHECK_FALSE(e1.Curr.x > e3.Curr.x); // No intersection of points!
    // Next scanbeam would reach 9
    top_y = 9;
    e1.Curr.y = top_y;
    e1.Curr.x = TopX(e1, top_y);
    e2.Curr.y = top_y;
    e2.Curr.x = TopX(e2, top_y);
    e3.Curr.y = top_y;
    e3.Curr.x = TopX(e3, top_y);
    CHECK(e1.Curr.x == 5);
    CHECK(e1.Curr.y == 9);
    CHECK(e2.Curr.x == 5);
    CHECK(e2.Curr.y == 9);
    CHECK(e3.Curr.x == 4);
    CHECK(e3.Curr.y == 9);
    // The active edge list would be e1, e2, e3
    CHECK_FALSE(e1.Curr.x > e2.Curr.x); // No intersection of points
    CHECK(e1.Curr.x > e3.Curr.x); // Intersection!
    intersection_point(e1, e3, r1);

    CHECK(r1.x == 3);
    CHECK(r1.y == 7);
}
