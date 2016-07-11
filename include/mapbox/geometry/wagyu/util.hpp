#pragma once

#include <cmath>

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
double area(mapbox::geometry::linear_ring<T> const& poly) {
    std::size_t size = poly.size();
    if (size < 3) {
        return 0.0;
    }

    double a = 0.0;
    auto itr = poly.begin();
    auto itr_prev = poly.end();
    --itr_prev;
    a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    ++itr;
    itr_prev = poly.begin();
    for (; itr != poly.end(); ++itr, ++itr_prev) {
        a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    }
    return -a * 0.5;
}

template <typename T>
bool orientation(mapbox::geometry::linear_ring<T> const& poly) {
    return area(poly) >= 0;
}

template <typename T>
std::size_t ring_depth(ring_ptr<T> r) {
    std::size_t count = 0;
    while (r) {
        ring_ptr<T> first_left = r->first_left;
        while (first_left && !first_left->points) {
            first_left = first_left->first_left;
        }
        ++count;
        r = first_left;
    }
    return count;
}

inline bool is_odd(std::size_t val) {
    return val & 1;
}

template <typename T>
bool slopes_equal(edge<T> const& e1, edge<T> const& e2) {
    return (e1.top.y - e1.bot.y) * (e2.top.x - e2.bot.x) ==
           (e1.top.x - e1.bot.x) * (e2.top.y - e2.bot.y);
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

template <typename T>
inline bool is_horizontal(edge<T> const& e) {
    return std::isinf(e.dx);
}

template <typename T>
bool is_even_odd_fill_type(bound<T> const& bound,
                           fill_type subject_fill_type,
                           fill_type clip_fill_type) {
    if (bound.poly_type == polygon_type_subject) {
        return subject_fill_type == fill_type_even_odd;
    } else {
        return clip_fill_type == fill_type_even_odd;
    }
}

template <typename T>
bool is_even_odd_alt_fill_type(bound<T> const& bound,
                               fill_type subject_fill_type,
                               fill_type clip_fill_type) {
    if (bound.poly_type == polygon_type_subject) {
        return clip_fill_type == fill_type_even_odd;
    } else {
        return subject_fill_type == fill_type_even_odd;
    }
}

template <typename T>
inline T get_current_x(edge<T> const& edge, const T current_y) {
    if (current_y == edge.top.y) {
        return edge.top.x;
    } else {
        return edge.bot.x +
               static_cast<T>(std::round(edge.dx * static_cast<double>(current_y - edge.bot.y)));
    }
}

template <typename T>
void swap_points(mapbox::geometry::point<T>& pt1, mapbox::geometry::point<T>& pt2) {
    mapbox::geometry::point<T> tmp = pt1;
    pt1 = pt2;
    pt2 = tmp;
}

template <typename T>
bool get_overlap_segment(mapbox::geometry::point<T> pt1a,
                         mapbox::geometry::point<T> pt1b,
                         mapbox::geometry::point<T> pt2a,
                         mapbox::geometry::point<T> pt2b,
                         mapbox::geometry::point<T>& pt1,
                         mapbox::geometry::point<T>& pt2) {
    // precondition: segments are Collinear.
    if (std::abs(pt1a.x - pt1b.x) > std::abs(pt1a.y - pt1b.y)) {
        if (pt1a.x > pt1b.x) {
            swap_points(pt1a, pt1b);
        }
        if (pt2a.x > pt2b.x) {
            swap_points(pt2a, pt2b);
        }
        if (pt1a.x > pt2a.x) {
            pt1 = pt1a;
        } else {
            pt1 = pt2a;
        }
        if (pt1b.x < pt2b.x) {
            pt2 = pt1b;
        } else {
            pt2 = pt2b;
        }
        return pt1.x < pt2.x;
    } else {
        if (pt1a.y < pt1b.y) {
            swap_points(pt1a, pt1b);
        }
        if (pt2a.y < pt2b.y) {
            swap_points(pt2a, pt2b);
        }
        if (pt1a.y < pt2a.y) {
            pt1 = pt1a;
        } else {
            pt1 = pt2a;
        }
        if (pt1b.y > pt2b.y) {
            pt2 = pt1b;
        } else {
            pt2 = pt2b;
        }
        return pt1.y > pt2.y;
    }
}

template <typename T>
bool point_2_is_between_point_1_and_point_3(mapbox::geometry::wagyu::point<T> pt1,
                                            mapbox::geometry::wagyu::point<T> pt2,
                                            mapbox::geometry::wagyu::point<T> pt3) {
    if ((pt1 == pt3) || (pt1 == pt2) || (pt3 == pt2)) {
        return false;
    } else if (pt1.x != pt3.x) {
        return (pt2.x > pt1.x) == (pt2.x < pt3.x);
    } else {
        return (pt2.y > pt1.y) == (pt2.y < pt3.y);
    }
}
}
}
}
