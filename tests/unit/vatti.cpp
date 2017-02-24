#include "catch.hpp"

#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/wagyu.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("no input test") {

    mapbox::geometry::wagyu::wagyu<std::int64_t> wagyu;

    mapbox::geometry::multi_polygon<std::int64_t> solution;
    CHECK_FALSE(wagyu.execute(mapbox::geometry::wagyu::clip_type_union, solution,
                              mapbox::geometry::wagyu::fill_type_positive,
                              mapbox::geometry::wagyu::fill_type_positive));
}

TEST_CASE("simple test for winding order - positive") {

    // This ring is counter-clockwise
    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 0));
    ring.push_back(mapbox::geometry::point<std::int64_t>(1, 0));
    ring.push_back(mapbox::geometry::point<std::int64_t>(1, 1));
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 1));
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 0));

    mapbox::geometry::wagyu::wagyu<std::int64_t> wagyu;
    wagyu.add_ring(ring);

    mapbox::geometry::multi_polygon<std::int64_t> solution;
    wagyu.execute(mapbox::geometry::wagyu::clip_type_union, solution,
                  mapbox::geometry::wagyu::fill_type_positive,
                  mapbox::geometry::wagyu::fill_type_positive);

    REQUIRE(solution.size() == 1);
    // Check first polygon number of rings
    REQUIRE(solution[0].size() == 1);

    // Check Ring 1 is counter clockwise as well
    REQUIRE(solution[0][0].size() == 5);
    CHECK(solution[0][0][0].x == 1);
    CHECK(solution[0][0][0].y == 0);

    CHECK(solution[0][0][1].x == 1);
    CHECK(solution[0][0][1].y == 1);

    CHECK(solution[0][0][2].x == 0);
    CHECK(solution[0][0][2].y == 1);

    CHECK(solution[0][0][3].x == 0);
    CHECK(solution[0][0][3].y == 0);

    CHECK(solution[0][0][4].x == 1);
    CHECK(solution[0][0][4].y == 0);
}

TEST_CASE("simple test for winding order - negative") {
    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 0));
    ring.push_back(mapbox::geometry::point<std::int64_t>(1, 0));
    ring.push_back(mapbox::geometry::point<std::int64_t>(1, 1));
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 1));
    ring.push_back(mapbox::geometry::point<std::int64_t>(0, 0));

    mapbox::geometry::wagyu::wagyu<std::int64_t> wagyu;
    wagyu.add_ring(ring);

    mapbox::geometry::multi_polygon<std::int64_t> solution;
    wagyu.execute(mapbox::geometry::wagyu::clip_type_union, solution,
                  mapbox::geometry::wagyu::fill_type_negative,
                  mapbox::geometry::wagyu::fill_type_negative);

    REQUIRE(solution.size() == 0);
}

TEST_CASE("simple test of entire vatti") {

    mapbox::geometry::wagyu::wagyu<T> clipper;
    mapbox::geometry::polygon<T> polygon0;
    mapbox::geometry::linear_ring<T> ring0_0;
    ring0_0.push_back({ -79102, 0 });
    ring0_0.push_back({ -70312, -55285 });
    ring0_0.push_back({ 85254, -30747 });
    ring0_0.push_back({ 58008, 80592 });
    ring0_0.push_back({ -79102, 0 });
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T> ring0_1;
    ring0_1.push_back({ 44824, 42149 });
    ring0_1.push_back({ 51855, -21089 });
    ring0_1.push_back({ -65918, -32502 });
    ring0_1.push_back({ -50098, 4394 });
    ring0_1.push_back({ 44824, 42149 });
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);

    mapbox::geometry::polygon<T> polygon1;
    mapbox::geometry::linear_ring<T> ring1_0;
    ring1_0.push_back({ 31201, 8349 });
    ring1_0.push_back({ 4834, 19771 });
    ring1_0.push_back({ -25488, -6592 });
    ring1_0.push_back({ 10547, -19771 });
    ring1_0.push_back({ 31201, 8349 });
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);

    mapbox::geometry::polygon<T> polygon2;
    mapbox::geometry::linear_ring<T> ring2_0;
    ring2_0.push_back({ -40430, -3076 });
    ring2_0.push_back({ -26367, -18454 });
    ring2_0.push_back({ 34277, -4834 });
    ring2_0.push_back({ 33838, 17136 });
    ring2_0.push_back({ -40430, -3076 });
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);

    mapbox::geometry::multi_polygon<T> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);

    REQUIRE(solution.size() == 2);
    // Check first polygon number of rings
    REQUIRE(solution[0].size() == 2);
    // Check second polygon number of rings
    REQUIRE(solution[1].size() == 1);

    // Check Ring 1
    REQUIRE(solution[0][0].size() == 5);
    CHECK(solution[0][0][0].x == -70312);
    CHECK(solution[0][0][0].y == -55285);
    CHECK(solution[0][0][1].x == 85254);
    CHECK(solution[0][0][1].y == -30747);
    CHECK(solution[0][0][2].x == 58008);
    CHECK(solution[0][0][2].y == 80592);
    CHECK(solution[0][0][3].x == -79102);
    CHECK(solution[0][0][3].y == 0);
    CHECK(solution[0][0][4].x == -70312);
    CHECK(solution[0][0][4].y == -55285);

    REQUIRE(solution[0][1].size() == 5);
    CHECK(solution[0][1][0].x == -65918);
    CHECK(solution[0][1][0].y == -32502);
    CHECK(solution[0][1][1].x == -50098);
    CHECK(solution[0][1][1].y == 4394);
    CHECK(solution[0][1][2].x == 44824);
    CHECK(solution[0][1][2].y == 42149);
    CHECK(solution[0][1][3].x == 51855);
    CHECK(solution[0][1][3].y == -21089);
    CHECK(solution[0][1][4].x == -65918);
    CHECK(solution[0][1][4].y == -32502);

    CHECK(solution[1][0].size() == 11);
}

TEST_CASE("simple test of entire vatti - reverse output") {

    mapbox::geometry::wagyu::wagyu<T> clipper;
    mapbox::geometry::polygon<T> polygon0;
    mapbox::geometry::linear_ring<T> ring0_0;
    ring0_0.push_back({ -79102, 0 });
    ring0_0.push_back({ -70312, -55285 });
    ring0_0.push_back({ 85254, -30747 });
    ring0_0.push_back({ 58008, 80592 });
    ring0_0.push_back({ -79102, 0 });
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T> ring0_1;
    ring0_1.push_back({ 44824, 42149 });
    ring0_1.push_back({ 51855, -21089 });
    ring0_1.push_back({ -65918, -32502 });
    ring0_1.push_back({ -50098, 4394 });
    ring0_1.push_back({ 44824, 42149 });
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);

    mapbox::geometry::polygon<T> polygon1;
    mapbox::geometry::linear_ring<T> ring1_0;
    ring1_0.push_back({ 31201, 8349 });
    ring1_0.push_back({ 4834, 19771 });
    ring1_0.push_back({ -25488, -6592 });
    ring1_0.push_back({ 10547, -19771 });
    ring1_0.push_back({ 31201, 8349 });
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);

    mapbox::geometry::polygon<T> polygon2;
    mapbox::geometry::linear_ring<T> ring2_0;
    ring2_0.push_back({ -40430, -3076 });
    ring2_0.push_back({ -26367, -18454 });
    ring2_0.push_back({ 34277, -4834 });
    ring2_0.push_back({ 33838, 17136 });
    ring2_0.push_back({ -40430, -3076 });
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);

    mapbox::geometry::multi_polygon<T> solution;
    clipper.reverse_rings(true);
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);

    REQUIRE(solution.size() == 2);
    // Check first polygon number of rings
    REQUIRE(solution[0].size() == 2);
    // Check second polygon number of rings
    REQUIRE(solution[1].size() == 1);

    // Check Ring 1
    REQUIRE(solution[0][0].size() == 5);
    CHECK(solution[0][0][0].x == -70312);
    CHECK(solution[0][0][0].y == -55285);
    CHECK(solution[0][0][1].x == -79102);
    CHECK(solution[0][0][1].y == 0);
    CHECK(solution[0][0][2].x == 58008);
    CHECK(solution[0][0][2].y == 80592);
    CHECK(solution[0][0][3].x == 85254);
    CHECK(solution[0][0][3].y == -30747);
    CHECK(solution[0][0][4].x == -70312);
    CHECK(solution[0][0][4].y == -55285);

    REQUIRE(solution[0][1].size() == 5);
    CHECK(solution[0][1][0].x == -65918);
    CHECK(solution[0][1][0].y == -32502);
    CHECK(solution[0][1][1].x == 51855);
    CHECK(solution[0][1][1].y == -21089);
    CHECK(solution[0][1][2].x == 44824);
    CHECK(solution[0][1][2].y == 42149);
    CHECK(solution[0][1][3].x == -50098);
    CHECK(solution[0][1][3].y == 4394);
    CHECK(solution[0][1][4].x == -65918);
    CHECK(solution[0][1][4].y == -32502);

    CHECK(solution[1][0].size() == 11);
}

TEST_CASE("simple test of entire vatti -- ring input and output as int32_t") {

    using T2 = std::int32_t;

    mapbox::geometry::wagyu::wagyu<T> clipper;
    mapbox::geometry::polygon<T2> polygon0;
    mapbox::geometry::linear_ring<T2> ring0_0;
    ring0_0.push_back({ -79102, 0 });
    ring0_0.push_back({ -70312, -55285 });
    ring0_0.push_back({ 85254, -30747 });
    ring0_0.push_back({ 58008, 80592 });
    ring0_0.push_back({ -79102, 0 });
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T2> ring0_1;
    ring0_1.push_back({ 44824, 42149 });
    ring0_1.push_back({ 51855, -21089 });
    ring0_1.push_back({ -65918, -32502 });
    ring0_1.push_back({ -50098, 4394 });
    ring0_1.push_back({ 44824, 42149 });
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);

    mapbox::geometry::polygon<T2> polygon1;
    mapbox::geometry::linear_ring<T2> ring1_0;
    ring1_0.push_back({ 31201, 8349 });
    ring1_0.push_back({ 4834, 19771 });
    ring1_0.push_back({ -25488, -6592 });
    ring1_0.push_back({ 10547, -19771 });
    ring1_0.push_back({ 31201, 8349 });
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);

    mapbox::geometry::polygon<T2> polygon2;
    mapbox::geometry::linear_ring<T2> ring2_0;
    ring2_0.push_back({ -40430, -3076 });
    ring2_0.push_back({ -26367, -18454 });
    ring2_0.push_back({ 34277, -4834 });
    ring2_0.push_back({ 33838, 17136 });
    ring2_0.push_back({ -40430, -3076 });
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);

    mapbox::geometry::multi_polygon<T2> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);

    REQUIRE(solution.size() == 2);
    // Check first polygon number of rings
    REQUIRE(solution[0].size() == 2);
    // Check second polygon number of rings
    REQUIRE(solution[1].size() == 1);

    // Check Ring 1
    REQUIRE(solution[0][0].size() == 5);
    CHECK(solution[0][0][0].x == -70312);
    CHECK(solution[0][0][0].y == -55285);
    CHECK(solution[0][0][1].x == 85254);
    CHECK(solution[0][0][1].y == -30747);
    CHECK(solution[0][0][2].x == 58008);
    CHECK(solution[0][0][2].y == 80592);
    CHECK(solution[0][0][3].x == -79102);
    CHECK(solution[0][0][3].y == 0);
    CHECK(solution[0][0][4].x == -70312);
    CHECK(solution[0][0][4].y == -55285);

    REQUIRE(solution[0][1].size() == 5);
    CHECK(solution[0][1][0].x == -65918);
    CHECK(solution[0][1][0].y == -32502);
    CHECK(solution[0][1][1].x == -50098);
    CHECK(solution[0][1][1].y == 4394);
    CHECK(solution[0][1][2].x == 44824);
    CHECK(solution[0][1][2].y == 42149);
    CHECK(solution[0][1][3].x == 51855);
    CHECK(solution[0][1][3].y == -21089);
    CHECK(solution[0][1][4].x == -65918);
    CHECK(solution[0][1][4].y == -32502);

    CHECK(solution[1][0].size() == 11);
}

TEST_CASE("simple test of entire vatti -- all processing as int32_t") {

    using T2 = std::int32_t;

    mapbox::geometry::wagyu::wagyu<T2> clipper;
    mapbox::geometry::polygon<T2> polygon0;
    mapbox::geometry::linear_ring<T2> ring0_0;
    ring0_0.push_back({ -79102, 0 });
    ring0_0.push_back({ -70312, -55285 });
    ring0_0.push_back({ 85254, -30747 });
    ring0_0.push_back({ 58008, 80592 });
    ring0_0.push_back({ -79102, 0 });
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T2> ring0_1;
    ring0_1.push_back({ 44824, 42149 });
    ring0_1.push_back({ 51855, -21089 });
    ring0_1.push_back({ -65918, -32502 });
    ring0_1.push_back({ -50098, 4394 });
    ring0_1.push_back({ 44824, 42149 });
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);

    mapbox::geometry::polygon<T2> polygon1;
    mapbox::geometry::linear_ring<T2> ring1_0;
    ring1_0.push_back({ 31201, 8349 });
    ring1_0.push_back({ 4834, 19771 });
    ring1_0.push_back({ -25488, -6592 });
    ring1_0.push_back({ 10547, -19771 });
    ring1_0.push_back({ 31201, 8349 });
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);

    mapbox::geometry::polygon<T2> polygon2;
    mapbox::geometry::linear_ring<T2> ring2_0;
    ring2_0.push_back({ -40430, -3076 });
    ring2_0.push_back({ -26367, -18454 });
    ring2_0.push_back({ 34277, -4834 });
    ring2_0.push_back({ 33838, 17136 });
    ring2_0.push_back({ -40430, -3076 });
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);

    mapbox::geometry::multi_polygon<T2> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);

    REQUIRE(solution.size() == 2);
    // Check first polygon number of rings
    REQUIRE(solution[0].size() == 2);
    // Check second polygon number of rings
    REQUIRE(solution[1].size() == 1);

    // Check Ring 1
    REQUIRE(solution[0][0].size() == 5);
    CHECK(solution[0][0][0].x == -70312);
    CHECK(solution[0][0][0].y == -55285);
    CHECK(solution[0][0][1].x == 85254);
    CHECK(solution[0][0][1].y == -30747);
    CHECK(solution[0][0][2].x == 58008);
    CHECK(solution[0][0][2].y == 80592);
    CHECK(solution[0][0][3].x == -79102);
    CHECK(solution[0][0][3].y == 0);
    CHECK(solution[0][0][4].x == -70312);
    CHECK(solution[0][0][4].y == -55285);

    REQUIRE(solution[0][1].size() == 5);
    CHECK(solution[0][1][0].x == -65918);
    CHECK(solution[0][1][0].y == -32502);
    CHECK(solution[0][1][1].x == -50098);
    CHECK(solution[0][1][1].y == 4394);
    CHECK(solution[0][1][2].x == 44824);
    CHECK(solution[0][1][2].y == 42149);
    CHECK(solution[0][1][3].x == 51855);
    CHECK(solution[0][1][3].y == -21089);
    CHECK(solution[0][1][4].x == -65918);
    CHECK(solution[0][1][4].y == -32502);

    CHECK(solution[1][0].size() == 11);
}
