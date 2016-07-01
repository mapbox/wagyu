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
bool PointIsVertex(mapbox::geometry::point<T> const& Pt, point_ptr<T> pp) {
    point_ptr<T> pp2 = pp;
    do {
        if (*pp2 == Pt) {
            return true;
        }
        pp2 = pp2->next;
    } while (pp2 != pp);
    return false;
}

enum point_in_polygon_result : std::int8_t {
    point_on_polygon = -1,
    point_inside_polygon = 0,
    point_outside_polygon = 1
};

template <typename T>
point_in_polygon_result point_in_polygon(point<T> const& pt,
                                         mapbox::geometry::linear_ring<T> const& path) {
    // returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    std::size_t cnt = path.size();
    if (cnt < 3) {
        return point_outside_polygon;
    }
    point_in_polygon_result result = point_outside_polygon;
    auto itr = path.begin();
    auto itr_prev = path.end();
    --itr_prev;
    if (itr->y == pt.y) {
        if ((itr->x == pt.x) ||
            (itr_prev->y == pt.y && ((itr->x > pt.x) == (itr_prev->x < pt.x)))) {
            return point_on_polygon;
        }
    }
    if ((itr_prev->y < pt.y) != (itr->y < pt.y)) {
        if (itr_prev->x >= pt.x) {
            if (itr->x > pt.x) {
                // Switch between point outside polygon and point inside polygon
                if (result == point_outside_polygon) {
                    result = point_inside_polygon;
                } else {
                    result = point_outside_polygon;
                }
            } else {
                double d =
                    static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) -
                    static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                if (d <= 0) {
                    return point_on_polygon;
                }
                if ((d > 0) == (itr->y > itr_prev->y)) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                }
            }
        } else {
            if (itr->x > pt.x) {
                double d =
                    static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) -
                    static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                if (d <= 0) {
                    return point_on_polygon;
                }
                if ((d > 0) == (itr->y > itr_prev->y)) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                }
            }
        }
    }
    ++itr;
    itr_prev = path.begin();
    for (; itr != path.end(); ++itr, ++itr_prev) {
        if (itr->y == pt.y) {
            if ((itr->x == pt.x) ||
                (itr_prev->y == pt.y && ((itr->x > pt.x) == (itr_prev->x < pt.x)))) {
                return point_on_polygon;
            }
        }
        if ((itr_prev->y < pt.y) != (itr->y < pt.y)) {
            if (itr_prev->x >= pt.x) {
                if (itr->x > pt.x) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                } else {
                    double d = static_cast<double>(itr_prev->x - pt.x) *
                                   static_cast<double>(itr->y - pt.y) -
                               static_cast<double>(itr->x - pt.x) *
                                   static_cast<double>(itr_prev->y - pt.y);
                    if (d <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (itr->y > itr_prev->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            } else {
                if (itr->x > pt.x) {
                    double d = static_cast<double>(itr_prev->x - pt.x) *
                                   static_cast<double>(itr->y - pt.y) -
                               static_cast<double>(itr->x - pt.x) *
                                   static_cast<double>(itr_prev->y - pt.y);
                    if (d <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (itr->y > itr_prev->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            }
        }
    }
    return result;
}

template <typename T>
point_in_polygon_result point_in_polygon(point<T> const& pt, point_ptr<T> op) {
    // returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    point_in_polygon_result result = point_outside_polygon;
    point_ptr<T> startOp = op;
    do {
        if (op->next->y == pt.y) {
            if ((op->next->x == pt.x) ||
                (op->y == pt.y && ((op->next->x > pt.x) == (op->x < pt.x)))) {
                return point_on_polygon;
            }
        }
        if ((op->y < pt.y) != (op->next->y < pt.y)) {
            if (op->x >= pt.x) {
                if (op->next->x > pt.x) {
                    // Switch between point outside polygon and point inside
                    // polygon
                    if (result == point_outside_polygon) {
                        result = point_inside_polygon;
                    } else {
                        result = point_outside_polygon;
                    }
                } else {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (d <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            } else {
                if (op->next->x > pt.x) {
                    double d =
                        static_cast<double>(op->x - pt.x) *
                            static_cast<double>(op->next->y - pt.y) -
                        static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (d <= 0) {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y)) {
                        // Switch between point outside polygon and point inside
                        // polygon
                        if (result == point_outside_polygon) {
                            result = point_inside_polygon;
                        } else {
                            result = point_outside_polygon;
                        }
                    }
                }
            }
        }
        op = op->next;
    } while (startOp != op);
    return result;
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
