#include "catch.hpp"

#include <mapbox/geometry/wagyu/wagyu.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("test returns zero with no data provided - int64") {
    mapbox::geometry::wagyu::wagyu<T> clipper;
    auto bounds = clipper.get_bounds();
    CHECK(bounds.min.x == 0);
    CHECK(bounds.min.y == 0);
    CHECK(bounds.max.x == 0);
    CHECK(bounds.max.y == 0);
}

TEST_CASE("test returns simple box - int64") {
    mapbox::geometry::polygon<T> polygon;
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });
    polygon.push_back(ring);
    mapbox::geometry::wagyu::wagyu<T> clipper;
    CHECK(clipper.add_polygon(polygon));
    auto bounds = clipper.get_bounds();
    CHECK(bounds.min.x == 0);
    CHECK(bounds.min.y == 0);
    CHECK(bounds.max.x == 5);
    CHECK(bounds.max.y == 5);
}

TEST_CASE("test returns simple box negative - int64") {
    mapbox::geometry::polygon<T> polygon;
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, -5 });
    ring.push_back({ -5, -5 });
    ring.push_back({ -5, 0 });
    ring.push_back({ 0, 0 });
    polygon.push_back(ring);
    mapbox::geometry::wagyu::wagyu<T> clipper;
    CHECK(clipper.add_polygon(polygon));
    auto bounds = clipper.get_bounds();
    CHECK(bounds.min.x == -5);
    CHECK(bounds.min.y == -5);
    CHECK(bounds.max.x == 0);
    CHECK(bounds.max.y == 0);
}

TEST_CASE("two polygons - int64") {
    mapbox::geometry::wagyu::wagyu<T> clipper;
    // Polygon 1
    mapbox::geometry::polygon<T> polygon;
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });
    polygon.push_back(ring);
    CHECK(clipper.add_polygon(polygon));
    polygon.clear();
    ring.clear();
    // Polygon 2
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 10 });
    ring.push_back({ 10, 10 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 5 });
    polygon.push_back(ring);
    CHECK(clipper.add_polygon(polygon));

    auto bounds = clipper.get_bounds();
    CHECK(bounds.min.x == 0);
    CHECK(bounds.min.y == 0);
    CHECK(bounds.max.x == 10);
    CHECK(bounds.max.y == 10);
}
