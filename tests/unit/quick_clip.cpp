#include <iostream>
#include "catch.hpp"

#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <mapbox/geometry/wagyu/quick_clip.hpp>

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const mapbox::geometry::linear_ring<T>& ring) {
    out << "[";
    bool first = true;
    for (auto const& pt: ring) {
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    CHECK(*out == lr);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(25, 25));
    lr2.push_back(mapbox::geometry::point<T>(100, 25));
    lr2.push_back(mapbox::geometry::point<T>(100, 75));
    lr2.push_back(mapbox::geometry::point<T>(25, 75));
    lr2.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(*out == lr2);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(0, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 75));
    lr2.push_back(mapbox::geometry::point<T>(0, 75));
    lr2.push_back(mapbox::geometry::point<T>(0, 25));

    CHECK(*out == lr2);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(25, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(*out == lr2);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(0, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 25));
    lr2.push_back(mapbox::geometry::point<T>(75, 75));
    lr2.push_back(mapbox::geometry::point<T>(0, 75));
    lr2.push_back(mapbox::geometry::point<T>(0, 25));

    CHECK(*out == lr2);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(25, 25));
    lr2.push_back(mapbox::geometry::point<T>(100, 25));
    lr2.push_back(mapbox::geometry::point<T>(100, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 25));

    CHECK(*out == lr2);
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

    optional_linear_ring<T> out = mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);

    mapbox::geometry::linear_ring<T> lr2;
    lr2.push_back(mapbox::geometry::point<T>(25, 0));
    lr2.push_back(mapbox::geometry::point<T>(100, 0));
    lr2.push_back(mapbox::geometry::point<T>(100, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 100));
    lr2.push_back(mapbox::geometry::point<T>(25, 0));

    std::cerr << "want " << lr2 << "\n";
    std::cerr << "got  " << *out << "\n";

    CHECK(*out == lr2);
}
