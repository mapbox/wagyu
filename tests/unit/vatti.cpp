#include "catch.hpp"

#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/wagyu.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("simple test of entire vatti") {
    // mapbox::geometry::polygon<T> polygon;
    // mapbox::geometry::linear_ring<T> ring;
    // ring.push_back({ 0, 0 });
    // ring.push_back({ 1000, 5000 });
    // ring.push_back({ 4000, 5200 });
    // ring.push_back({ 1000, 1000 });
    // ring.push_back({ 5500, 4000 });
    // ring.push_back({ 5000, 500 });
    // ring.push_back({ 0, 0 });
    // polygon.push_back(ring);

    // mapbox::geometry::polygon<T> polygon2;
    // mapbox::geometry::linear_ring<T> ring2;
    // ring2.push_back({ 2000, 2000 });
    // ring2.push_back({ 1000, 7000 });
    // ring2.push_back({ 6000, 7500 });
    // ring2.push_back({ 7000, 2500 });
    // ring2.push_back({ 2000, 2000 });
    // polygon2.push_back(ring2);

    mapbox::geometry::wagyu::clipper<T> clipper;
    // CHECK(clipper.add_polygon(polygon));
    // clipper.add_polygon(polygon2, polygon_type::polygon_type_clip);
    mapbox::geometry::polygon<T> polygon0;
    mapbox::geometry::linear_ring<T> ring0_0;
    ring0_0.push_back({-79102, 0});
    ring0_0.push_back({-70312, -55285});
    ring0_0.push_back({85254, -30747});
    ring0_0.push_back({58008, 80592});
    ring0_0.push_back({-79102, 0});
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T> ring0_1;
    ring0_1.push_back({44824, 42149});
    ring0_1.push_back({51855, -21089});
    ring0_1.push_back({-65918, -32502});
    ring0_1.push_back({-50098, 4394});
    ring0_1.push_back({44824, 42149});
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);


    mapbox::geometry::polygon<T> polygon1;
    mapbox::geometry::linear_ring<T> ring1_0;
    ring1_0.push_back({31201, 8349});
    ring1_0.push_back({4834, 19771});
    ring1_0.push_back({-25488, -6592});
    ring1_0.push_back({10547, -19771});
    ring1_0.push_back({31201, 8349});
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);


    mapbox::geometry::polygon<T> polygon2;
    mapbox::geometry::linear_ring<T> ring2_0;
    ring2_0.push_back({-40430, -3076});
    ring2_0.push_back({-26367, -18454});
    ring2_0.push_back({34277, -4834});
    ring2_0.push_back({33838, 17136});
    ring2_0.push_back({-40430, -3076});
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);


    std::vector<mapbox::geometry::polygon<T>> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);
}
