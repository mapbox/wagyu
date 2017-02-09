#include "catch.hpp"
#include <iostream>

#include <mapbox/geometry/wagyu/quick_clip.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const mapbox::geometry::linear_ring<T>& ring) {
    out << "[";
    bool first = true;
    for (auto const& pt : ring) {
        if (first) {
            out << "[";
            first = false;
        } else {
            out << ",[";
        }
        out << pt.x << "," << pt.y << "]";
    }
    out << "]";
    return out;
}

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("square entirely within bbox") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    CHECK(out == lr);
}

TEST_CASE("square cut at right") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, 25));
    lr.push_back(mapbox::geometry::point<T>(175, 25));
    lr.push_back(mapbox::geometry::point<T>(175, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(25, 25));
    want.push_back(mapbox::geometry::point<T>(100, 25));
    want.push_back(mapbox::geometry::point<T>(100, 75));
    want.push_back(mapbox::geometry::point<T>(25, 75));
    want.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(out == want);
}

TEST_CASE("square cut at left") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(-25, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 75));
    lr.push_back(mapbox::geometry::point<T>(-25, 75));
    lr.push_back(mapbox::geometry::point<T>(-25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(0, 25));
    want.push_back(mapbox::geometry::point<T>(75, 25));
    want.push_back(mapbox::geometry::point<T>(75, 75));
    want.push_back(mapbox::geometry::point<T>(0, 75));
    want.push_back(mapbox::geometry::point<T>(0, 25));

    CHECK(out == want);
}

TEST_CASE("square cut at top") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 175));
    lr.push_back(mapbox::geometry::point<T>(25, 175));
    lr.push_back(mapbox::geometry::point<T>(25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(25, 25));
    want.push_back(mapbox::geometry::point<T>(75, 25));
    want.push_back(mapbox::geometry::point<T>(75, 100));
    want.push_back(mapbox::geometry::point<T>(25, 100));
    want.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(out == want);
}

TEST_CASE("square cut at bottom") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(-25, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 25));
    lr.push_back(mapbox::geometry::point<T>(75, 75));
    lr.push_back(mapbox::geometry::point<T>(-25, 75));
    lr.push_back(mapbox::geometry::point<T>(-25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(0, 25));
    want.push_back(mapbox::geometry::point<T>(75, 25));
    want.push_back(mapbox::geometry::point<T>(75, 75));
    want.push_back(mapbox::geometry::point<T>(0, 75));
    want.push_back(mapbox::geometry::point<T>(0, 25));

    CHECK(out == want);
}

TEST_CASE("square cut at top right") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, 25));
    lr.push_back(mapbox::geometry::point<T>(175, 25));
    lr.push_back(mapbox::geometry::point<T>(175, 175));
    lr.push_back(mapbox::geometry::point<T>(25, 175));
    lr.push_back(mapbox::geometry::point<T>(25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(25, 25));
    want.push_back(mapbox::geometry::point<T>(100, 25));
    want.push_back(mapbox::geometry::point<T>(100, 100));
    want.push_back(mapbox::geometry::point<T>(25, 100));
    want.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(out == want);
}

TEST_CASE("square cut at top and bottom right") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, -25));
    lr.push_back(mapbox::geometry::point<T>(175, -25));
    lr.push_back(mapbox::geometry::point<T>(175, 175));
    lr.push_back(mapbox::geometry::point<T>(25, 175));
    lr.push_back(mapbox::geometry::point<T>(25, -25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(100, 0));
    want.push_back(mapbox::geometry::point<T>(100, 100));
    want.push_back(mapbox::geometry::point<T>(25, 100));
    want.push_back(mapbox::geometry::point<T>(25, 0));
    want.push_back(mapbox::geometry::point<T>(100, 0));

    CHECK(out == want);
}

TEST_CASE("square entirely out of bounds") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(125, 125));
    lr.push_back(mapbox::geometry::point<T>(175, 125));
    lr.push_back(mapbox::geometry::point<T>(175, 175));
    lr.push_back(mapbox::geometry::point<T>(125, 175));
    lr.push_back(mapbox::geometry::point<T>(125, 125));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    CHECK(out.empty());
}

TEST_CASE("square entirely enclosing bbox") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(-25, -25));
    lr.push_back(mapbox::geometry::point<T>(175, -25));
    lr.push_back(mapbox::geometry::point<T>(175, 175));
    lr.push_back(mapbox::geometry::point<T>(-25, 175));
    lr.push_back(mapbox::geometry::point<T>(-25, -25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(0, 0));
    want.push_back(mapbox::geometry::point<T>(100, 0));
    want.push_back(mapbox::geometry::point<T>(100, 100));
    want.push_back(mapbox::geometry::point<T>(0, 100));
    want.push_back(mapbox::geometry::point<T>(0, 0));

    CHECK(out == want);
}

TEST_CASE("sticking out and back in") {
    mapbox::geometry::point<T> p1 = { 0, 0 };
    mapbox::geometry::point<T> p2 = { 100, 100 };
    mapbox::geometry::box<T> bbox(p1, p2);

    mapbox::geometry::linear_ring<T> lr;
    lr.push_back(mapbox::geometry::point<T>(25, 25));
    lr.push_back(mapbox::geometry::point<T>(150, 25));
    lr.push_back(mapbox::geometry::point<T>(150, 150));
    lr.push_back(mapbox::geometry::point<T>(25, 150));
    lr.push_back(mapbox::geometry::point<T>(25, 90));
    lr.push_back(mapbox::geometry::point<T>(75, 90));
    lr.push_back(mapbox::geometry::point<T>(75, 125));
    lr.push_back(mapbox::geometry::point<T>(125, 125));
    lr.push_back(mapbox::geometry::point<T>(125, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 75));
    lr.push_back(mapbox::geometry::point<T>(25, 25));

    auto out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> want;
    want.push_back(mapbox::geometry::point<T>(25, 25));
    want.push_back(mapbox::geometry::point<T>(100, 25));
    want.push_back(mapbox::geometry::point<T>(100, 100));
    want.push_back(mapbox::geometry::point<T>(25, 100));
    want.push_back(mapbox::geometry::point<T>(25, 90));
    want.push_back(mapbox::geometry::point<T>(75, 90));
    want.push_back(mapbox::geometry::point<T>(75, 100));
    want.push_back(mapbox::geometry::point<T>(100, 100));
    want.push_back(mapbox::geometry::point<T>(100, 75));
    want.push_back(mapbox::geometry::point<T>(25, 75));
    want.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(out == want);

    mapbox::geometry::wagyu::wagyu<T> clipper;
    clipper.add_ring(out);
    mapbox::geometry::multi_polygon<T> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);

    REQUIRE(solution.size() == 2);

    mapbox::geometry::linear_ring<T> want0;
    want0.push_back(mapbox::geometry::point<T>(75, 90));
    want0.push_back(mapbox::geometry::point<T>(75, 100));
    want0.push_back(mapbox::geometry::point<T>(25, 100));
    want0.push_back(mapbox::geometry::point<T>(25, 90));
    want0.push_back(mapbox::geometry::point<T>(75, 90));

    mapbox::geometry::linear_ring<T> want1;
    want1.push_back(mapbox::geometry::point<T>(100, 75));
    want1.push_back(mapbox::geometry::point<T>(25, 75));
    want1.push_back(mapbox::geometry::point<T>(25, 25));
    want1.push_back(mapbox::geometry::point<T>(100, 25));
    want1.push_back(mapbox::geometry::point<T>(100, 75));

    CHECK(solution[0][0] == want0);
    CHECK(solution[1][0] == want1);

    mapbox::geometry::polygon<T> poly;
    poly.push_back(lr);
    mapbox::geometry::multi_polygon<T> poly_out;
    poly_out = mapbox::geometry::wagyu::clip(poly, bbox, fill_type_even_odd);

    REQUIRE(poly_out.size() == 2);
    CHECK(poly_out[0][0] == want0);
    CHECK(poly_out[1][0] == want1);
}
