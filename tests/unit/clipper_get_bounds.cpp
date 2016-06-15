#include "catch.hpp"

#include <mapbox/geometry/wagyu/wagyu.hpp>

TEST_CASE("test returns zero with no data provided - int64")
{
    mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
    auto bounds = clipper.get_bounds();
    CHECK(bounds.left == 0);
    CHECK(bounds.top == 0);
    CHECK(bounds.right == 0);
    CHECK(bounds.bottom == 0);
}

TEST_CASE("test returns simple box - int64")
{
    mapbox::geometry::polygon<std::int64_t> polygon;
    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({0, 0});
    polygon.push_back(ring);
    mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
    CHECK(clipper.add_polygon(polygon));
    auto bounds = clipper.get_bounds();
    CHECK(bounds.left == 0);
    CHECK(bounds.top == 0);
    CHECK(bounds.right == 5);
    CHECK(bounds.bottom == 5);
}

TEST_CASE("test returns simple box negative - int64")
{
    mapbox::geometry::polygon<std::int64_t> polygon;
    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, -5});
    ring.push_back({-5, -5});
    ring.push_back({-5, 0});
    ring.push_back({0, 0});
    polygon.push_back(ring);
    mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
    CHECK(clipper.add_polygon(polygon));
    auto bounds = clipper.get_bounds();
    CHECK(bounds.left == -5);
    CHECK(bounds.top == -5);
    CHECK(bounds.right == 0);
    CHECK(bounds.bottom == 0);
}

TEST_CASE("two polygons - int64")
{
    mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
    // Polygon 1
    mapbox::geometry::polygon<std::int64_t> polygon;
    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({0, 0});
    polygon.push_back(ring);
    CHECK(clipper.add_polygon(polygon));
    polygon.clear();
    ring.clear();
    // Polygon 2
    ring.push_back({5, 5});
    ring.push_back({5, 10});
    ring.push_back({10, 10});
    ring.push_back({10, 5});
    ring.push_back({5, 5});
    polygon.push_back(ring);
    CHECK(clipper.add_polygon(polygon));

    auto bounds = clipper.get_bounds();
    CHECK(bounds.left == 0);
    CHECK(bounds.top == 0);
    CHECK(bounds.right == 10);
    CHECK(bounds.bottom == 10);
}
