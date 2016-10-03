#pragma once

#include <cmath>

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/point.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

inline bool value_is_zero(double val) {
    return std::fabs(val) < std::numeric_limits<double>::epsilon();
}

inline bool values_are_equal(double x, double y) {
    return value_is_zero(x - y);
}

inline bool values_near_equal(double x, double y) {
    return std::fabs(x - y) > (5.0 * std::numeric_limits<double>::epsilon());
}

inline bool greater_than_or_equal(double x, double y) {
    return x > y || values_are_equal(x, y);
}

inline bool less_than_or_equal(double x, double y) {
    return x < y || values_are_equal(x, y);
}

template <typename T>
bool slopes_equal(mapbox::geometry::point<T> const& pt1,
                  mapbox::geometry::point<T> const& pt2,
                  mapbox::geometry::point<T> const& pt3) {
    return (pt1.y - pt2.y) * (pt2.x - pt3.x) == (pt1.x - pt2.x) * (pt2.y - pt3.y);
}

template <typename T>
bool slopes_equal(mapbox::geometry::wagyu::point<T> const& pt1,
                  mapbox::geometry::wagyu::point<T> const& pt2,
                  mapbox::geometry::point<T> const& pt3) {
    return (pt1.y - pt2.y) * (pt2.x - pt3.x) == (pt1.x - pt2.x) * (pt2.y - pt3.y);
}

template <typename T>
bool slopes_equal(mapbox::geometry::wagyu::point<T> const& pt1,
                  mapbox::geometry::wagyu::point<T> const& pt2,
                  mapbox::geometry::wagyu::point<T> const& pt3) {
    return (pt1.y - pt2.y) * (pt2.x - pt3.x) == (pt1.x - pt2.x) * (pt2.y - pt3.y);
}

template <typename T>
bool slopes_equal(mapbox::geometry::point<T> const& pt1,
                  mapbox::geometry::point<T> const& pt2,
                  mapbox::geometry::point<T> const& pt3,
                  mapbox::geometry::point<T> const& pt4) {
    return (pt1.y - pt2.y) * (pt3.x - pt4.x) == (pt1.x - pt2.x) * (pt3.y - pt4.y);
}
}
}
}
