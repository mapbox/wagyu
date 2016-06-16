#include "catch.hpp"

#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/wagyu.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("simple test of entire vatti") {
    mapbox::geometry::polygon<T> polygon;
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });
    polygon.push_back(ring);
    mapbox::geometry::wagyu::clipper<T> clipper;
    CHECK(clipper.add_polygon(polygon));

    mapbox::geometry::polygon<T> result;

    clipper.execute(clip_type_union, result, fill_type_even_odd, fill_type_even_odd);
}
