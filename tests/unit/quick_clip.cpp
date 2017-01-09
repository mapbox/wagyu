#include <iostream>
#include "catch.hpp"

#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <mapbox/geometry/wagyu/quick_clip.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

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

    std::cerr << *out;
    std::cerr << lr2;

    CHECK(*out == lr2);
}
