#pragma once

#include <mapbox/geometry/box.hpp>
#include <mapbox/geometry/multi_polygon.hpp>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <experimental/optional>

template <typename T>
using optional_linear_ring = std::experimental::optional<mapbox::geometry::linear_ring<T>>;

namespace mapbox {
namespace geometry {
namespace wagyu {
namespace quick_clip {

template <typename T>
bool point_inside(mapbox::geometry::point<T> const& pt, mapbox::geometry::box<T> const& b) {
    return (pt.x >= b.min.x && pt.x <= b.max.x &&
            pt.y >= b.min.y && pt.y <= b.max.y);
}

template <typename T>
void add_point(mapbox::geometry::linear_ring<T> & ring, mapbox::geometry::point<T> const& pt) {
    if (ring.empty() || ring.back() != pt) {
        ring.push_back(pt);
    }
}

template <typename T>
void compute_intersection_x(T x,
                            T min_y,
                            T max_y,
                            mapbox::geometry::point<T> const& pt1,
                            mapbox::geometry::point<T> const& pt2,
                            mapbox::geometry::linear_ring<T> & new_pts) {
    // Since we are dealing with x constant value, this is
    // an intersection with a vertical line. Therefore, if 
    // dy == 0, this is another vertial line and therefore they
    // can not intersect
    T dy = pt2.y - pt1.y;
    if (dy == 0) {
        return;
    }
    T dx = pt2.x - pt1.x;
    T pt_min_y = std::min(pt2.y, pt1.y);
    T pt_max_y = std::max(pt2.y, pt1.y);
    double dydx = static_cast<double>(dy) / static_cast<double>(dx);
    T y = std::round(static_cast<double>(pt1.y) + dydx * static_cast<double>(x - pt1.x));
    if (y >= pt_min_y && y <= pt_max_y) {
        // There is an interception with the line during within this segment.
        // So lets add it to the new_pts
        if (y < min_y) {
            y = min_y;
        } else if (y > max_y) {
            y = max_y;
        }
        new_pts.emplace_back(x, y);
    }
}

template <typename T>
void compute_intersection_y(T y,
                            T min_x,
                            T max_x,
                            mapbox::geometry::point<T> const& pt1,
                            mapbox::geometry::point<T> const& pt2,
                            mapbox::geometry::linear_ring<T> & new_pts) {
    // Since we are dealing with x constant value, this is
    // an intersection with a horizontal line. Therefore, if 
    // dx == 0, this is another horizontal line and therefore they
    // can not intersect
    T dx = pt2.x - pt1.x;
    if (dx == 0) {
        return;
    }
    T dy = pt2.y - pt1.y;
    T pt_min_x = std::min(pt2.x, pt1.x);
    T pt_max_x = std::max(pt2.x, pt1.x);
    double dxdy = static_cast<double>(dx) / static_cast<double>(dy);
    T x = std::round(static_cast<double>(pt1.x) + dxdy * static_cast<double>(y - pt1.y));
    if (x >= pt_min_x && x <= pt_max_x) {
        // There is an interception with the line during within this segment.
        // So lets add it to the new_pts
        if (x < min_x) {
            x = min_x;
        } else if (x > max_x) {
            x = max_x;
        }
        new_pts.emplace_back(x, y);
    }
}

template <typename T>
void add_intersection_point(mapbox::geometry::linear_ring<T> & ring,
                            mapbox::geometry::box<T> const& b,
                            mapbox::geometry::point<T> const& pt1,
                            mapbox::geometry::point<T> const& pt2) {
    if (pt1 == pt2) {
        return;
    }
    T dx = pt2.x - pt1.x;
    T dy = pt2.y - pt1.y;
    mapbox::geometry::linear_ring<T> new_pts;
    if (dy > 0 || dy < 0) {
        compute_intersection_x(b.min.x, b.min.y, b.max.y, pt1, pt2, new_pts);
        compute_intersection_x(b.max.x, b.min.y, b.max.y, pt1, pt2, new_pts);
    }
    if (dx > 0 || dx < 0) {
        compute_intersection_y(b.min.y, b.min.x, b.max.x, pt1, pt2, new_pts);
        compute_intersection_y(b.max.y, b.min.x, b.max.x, pt1, pt2, new_pts);
    }
    if (new_pts.empty()) {
        return;
    }
    if (dx > 0) {
        // dx positive means we sort min to max
        std::sort(new_pts.begin(), new_pts.end(), 
                [](mapbox::geometry::point<T> const& a, mapbox::geometry::point<T> const& b) {
                    return a.x < b.x;
                });
    } else if (dx < 0) {
        std::sort(new_pts.begin(), new_pts.end(), 
                [](mapbox::geometry::point<T> const& a, mapbox::geometry::point<T> const& b) {
                    return a.x > b.x;
                });
    } else if (dy > 0) {
        // Because dx == 0 we fall back to dy for sorting
        std::sort(new_pts.begin(), new_pts.end(), 
                [](mapbox::geometry::point<T> const& a, mapbox::geometry::point<T> const& b) {
                    return a.y < b.y;
                });
    } else {
        // Because dx == 0 we fall back to dy for sorting
        std::sort(new_pts.begin(), new_pts.end(), 
                [](mapbox::geometry::point<T> const& a, mapbox::geometry::point<T> const& b) {
                    return a.y > b.y;
                });
    }
    for (auto const& pt : new_pts) {
        add_point(ring, pt);
    }
}

template <typename T>
optional_linear_ring<T> quick_lr_clip(mapbox::geometry::linear_ring<T> const& ring,
                                      mapbox::geometry::box<T> const& b) {
    if (ring.size() < 3) {
        return optional_linear_ring<T>();
    }
    mapbox::geometry::linear_ring<T> new_ring;
    auto itr_1 = ring.end() - 1;
    auto itr_2 = ring.begin();
    auto itr_3 = std::next(itr_2);
    while (itr_2 != ring.end()) {
        if (point_inside(*itr_2, b)) {
            add_point(new_ring, *itr_2);
        } else {
            add_intersection_point(new_ring, b, *itr_1, *itr_2);
            add_intersection_point(new_ring, b, *itr_2, *itr_3);
        }
        ++itr_1;
        ++itr_2;
        ++itr_3;
        if (itr_1 == ring.end()) {
            itr_1 = ring.begin();
        }
        if (itr_3 == ring.end()) {
            itr_3 = ring.begin();
        }
    }
    if (new_ring.size() < 3) {
        return optional_linear_ring<T>();
    }
    return optional_linear_ring<T>(std::move(new_ring));
}

}

template <typename T>
mapbox::geometry::multi_polygon<T> clip(mapbox::geometry::polygon<T> const& poly,
                               mapbox::geometry::box<T> const& b,
                               fill_type subject_fill_type) {
    mapbox::geometry::multi_polygon<T> result;
    wagyu<T> clipper;
    for (auto const& lr : poly) {
        auto new_lr = quick_clip::quick_lr_clip(lr, b);
        if (new_lr) {
            clipper.add_ring(*new_lr, polygon_type_subject);
        }
    }
    clipper.execute(clip_type_union, result, subject_fill_type, fill_type_even_odd);
    return result;
}

template <typename T>
mapbox::geometry::multi_polygon<T> clip(mapbox::geometry::multi_polygon<T> const& mp,
                               mapbox::geometry::box<T> const& b,
                               fill_type subject_fill_type) {
    mapbox::geometry::multi_polygon<T> result;
    wagyu<T> clipper;
    for (auto const& poly : mp) {
        for (auto const& lr : poly) {
            auto new_lr = quick_clip::quick_lr_clip(lr, b);
            if (new_lr) {
                clipper.add_ring(*new_lr, polygon_type_subject);
            }
        }
    }
    clipper.execute(clip_type_union, result, subject_fill_type, fill_type_even_odd);
    return result;
}

}
}
}
