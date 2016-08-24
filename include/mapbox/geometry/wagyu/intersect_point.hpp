#pragma once

#include <cmath>
#include <limits>

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
inline T get_current_min_x(edge<T> const& edge, const T current_y, mapbox::geometry::point<double> const& int_pt) {
    if (is_horizontal(edge)) {
        if (edge.bot.x < edge.top.x) {
            return edge.bot.x;
        } else {
            return edge.top.x;
        }
    } else if (edge.dx > 0.0) {
        if (current_y == edge.top.y) {
            return edge.top.x;
        } else {
            double lower_range_y = static_cast<double>(current_y - edge.bot.y) - 0.5;
            if (int_pt.y > (current_y - 0.5)) {
                lower_range_y = int_pt.y - edge.bot.y;
            }
            return edge.bot.x + static_cast<T>(std::round(edge.dx * lower_range_y));
        }
    } else {
        if (current_y == edge.bot.y) {
            return edge.bot.x;
        } else {
            return edge.bot.x + static_cast<T>(std::round(
                                    edge.dx * (static_cast<double>(current_y - edge.bot.y) + 0.5)));
        }
    }
}

template <typename T>
inline T get_current_max_x(edge<T> const& edge, const T current_y, mapbox::geometry::point<double> const& int_pt) {
    if (is_horizontal(edge)) {
        if (edge.bot.x > edge.top.x) {
            return edge.bot.x;
        } else {
            return edge.top.x;
        }
    } else if (edge.dx < 0.0) {
        if (current_y == edge.top.y) {
            return edge.top.x;
        } else {
            double lower_range_y = static_cast<double>(current_y - edge.bot.y) - 0.5;
            if (int_pt.y > (current_y - 0.5)) {
                lower_range_y = int_pt.y - edge.bot.y;
            }
            return edge.bot.x + static_cast<T>(std::round(edge.dx * lower_range_y));
        }
    } else {
        if (current_y == edge.bot.y) {
            return edge.bot.x;
        } else {
            return edge.bot.x + static_cast<T>(std::round(
                                    edge.dx * (static_cast<double>(current_y - edge.bot.y) + 0.5)));
        }
    }
}

template <typename T>
void find_nearest_shared_point_left(edge<T> const& e1,
                                    edge<T> const& e2,
                                    mapbox::geometry::point<T>& pt,
                                    hot_pixel_set<T>& hot_pixels,
                                    T bottom_y,
                                    mapbox::geometry::point<double> const& int_pt) {
    double b1 = e1.bot.x - e1.bot.y * e1.dx;
    double b2 = e2.bot.x - e2.bot.y * e2.dx;
    double q = (b2 - b1) / (e1.dx - e2.dx);
    pt.y = static_cast<T>(std::round(q));
    if (std::fabs(e1.dx) < std::fabs(e2.dx)) {
        pt.x = static_cast<T>(std::round(e1.dx * q + b1));
    } else {
        pt.x = static_cast<T>(std::round(e2.dx * q + b2));
    }
    T current_y = pt.y;
    auto pixel_at_least = [current_y](mapbox::geometry::point<T> const& p) {
        return p.y < current_y;
    };
    auto hot_pixel_itr = hot_pixels.begin();
    // The first time that the two pixel paths diverge we must end
    // our search, otherwise we are entering unknown territory. If
    // the previous set of min/max where pixels align does not touch
    // contain any of the same values of the previous min/max then we
    // must abort the search upwards. This is why we are tracking
    // the previous min.
    T previous_min = pt.x;
    // Search for locations where the "pixels" of both paths overlap
    for (;;) {
        if (current_y > e1.bot.y || current_y > e2.bot.y || current_y > bottom_y) {
            break;
        }
        hot_pixel_itr = std::find_if_not(hot_pixel_itr, hot_pixels.end(), pixel_at_least);
        T e1_min = get_current_min_x(e1, current_y, int_pt);
        T e1_max = get_current_max_x(e1, current_y, int_pt);
        T e2_min = get_current_min_x(e2, current_y, int_pt);
        T e2_max = get_current_max_x(e2, current_y, int_pt);
        T min;
        if (e1_min >= e2_min && e1_min <= e2_max) {
            min = e1_min;
        } else if (e2_min >= e1_min && e2_min <= e1_max) {
            min = e2_min;
        } else {
            break;
        }
        T max = std::min(e1_max, e2_max);
        if (previous_min > max) {
            break;
        }
        previous_min = min;
        // there is some overlap -- take left most
        // or the last hot pixel found in range
        pt.x = min;
        pt.y = current_y;
        bool hot_pixel_found = false;
        while (hot_pixel_itr != hot_pixels.end() && hot_pixel_itr->y == current_y) {
            if (hot_pixel_itr->x >= min && hot_pixel_itr->x <= max) {
                pt.x = hot_pixel_itr->x;
                hot_pixel_found = true;
            }
            ++hot_pixel_itr;
        }
        if (hot_pixel_found) {
            break;
        }
        ++current_y;
    }
}

template <typename T>
void find_nearest_shared_point_right(edge<T> const& e1,
                                     edge<T> const& e2,
                                     mapbox::geometry::point<T>& pt,
                                     hot_pixel_set<T>& hot_pixels,
                                     T bottom_y,
                                     mapbox::geometry::point<double> const& int_pt) {
    double b1 = e1.bot.x - e1.bot.y * e1.dx;
    double b2 = e2.bot.x - e2.bot.y * e2.dx;
    double q = (b2 - b1) / (e1.dx - e2.dx);
    pt.y = static_cast<T>(std::round(q));
    if (std::fabs(e1.dx) < std::fabs(e2.dx)) {
        pt.x = static_cast<T>(std::round(e1.dx * q + b1));
    } else {
        pt.x = static_cast<T>(std::round(e2.dx * q + b2));
    }
    T current_y = pt.y;
    auto pixel_at_least = [current_y](mapbox::geometry::point<T> const& p) {
        return p.y < current_y;
    };
    auto hot_pixel_itr = hot_pixels.begin();
    // The first time that the two pixel paths diverge we must end
    // our search, otherwise we are entering unknown territory. If
    // the previous set of min/max where pixels align does not touch
    // contain any of the same values of the previous min/max then we
    // must abort the search upwards. This is why we are tracking
    // the previous max.
    T previous_max = pt.x;
    // Search for locations where the "pixels" of both paths overlap
    for (;;) {
        if (current_y > e1.bot.y || current_y > e2.bot.y || current_y > bottom_y) {
            break;
        }
        hot_pixel_itr = std::find_if_not(hot_pixel_itr, hot_pixels.end(), pixel_at_least);
        T e1_min = get_current_min_x(e1, current_y, int_pt);
        T e1_max = get_current_max_x(e1, current_y, int_pt);
        T e2_min = get_current_min_x(e2, current_y, int_pt);
        T e2_max = get_current_max_x(e2, current_y, int_pt);
        T min;
        if (e1_min >= e2_min && e1_min <= e2_max) {
            min = e1_min;
        } else if (e2_min >= e1_min && e2_min <= e1_max) {
            min = e2_min;
        } else {
            break;
        }
        T max = std::min(e1_max, e2_max);
        if (previous_max < min) {
            break;
        }
        previous_max = max;
        // there is some overlap -- take right most.
        pt.x = max;
        pt.y = current_y;
        bool hot_pixel_found = false;
        while (hot_pixel_itr != hot_pixels.end() && hot_pixel_itr->y == current_y) {
            if (hot_pixel_itr->x >= min && hot_pixel_itr->x <= max) {
                pt.x = hot_pixel_itr->x;
                hot_pixel_found = true;
                break;
            }
            ++hot_pixel_itr;
        }
        if (hot_pixel_found) {
            break;
        }
        ++current_y;
    }
}

template <typename T>
void find_nearest_shared_point_center(edge<T> const& e1,
                                      edge<T> const& e2,
                                      mapbox::geometry::point<T>& pt,
                                      hot_pixel_set<T>& hot_pixels,
                                      T bottom_y, 
                                      mapbox::geometry::point<double> const& int_pt) {
    double b1 = e1.bot.x - e1.bot.y * e1.dx;
    double b2 = e2.bot.x - e2.bot.y * e2.dx;
    double q = (b2 - b1) / (e1.dx - e2.dx);
    pt.y = static_cast<T>(std::round(q));
    if (std::fabs(e1.dx) < std::fabs(e2.dx)) {
        pt.x = static_cast<T>(std::round(e1.dx * q + b1));
    } else {
        pt.x = static_cast<T>(std::round(e2.dx * q + b2));
    }
    T current_y = pt.y;
    auto pixel_at_least = [current_y](mapbox::geometry::point<T> const& p) {
        return p.y < current_y;
    };
    auto hot_pixel_itr = hot_pixels.begin();
    // The first time that the two pixel paths diverge we must end
    // our search, otherwise we are entering unknown territory. If
    // the previous set of min/max where pixels align does not touch
    // contain any of the same values of the previous min/max then we
    // must abort the search upwards. This is why we are tracking
    // the previous max and min
    T previous_min = pt.x;
    T previous_max = pt.x;
    // Search for locations where the "pixels" of both paths overlap
    for (;;) {
        if (current_y > e1.bot.y || current_y > e2.bot.y || current_y > bottom_y) {
            break;
        }
        hot_pixel_itr = std::find_if_not(hot_pixel_itr, hot_pixels.end(), pixel_at_least);
        T e1_min = get_current_min_x(e1, current_y, int_pt);
        T e1_max = get_current_max_x(e1, current_y, int_pt);
        T e2_min = get_current_min_x(e2, current_y, int_pt);
        T e2_max = get_current_max_x(e2, current_y, int_pt);
        T min;
        if (e1_min >= e2_min && e1_min <= e2_max) {
            min = e1_min;
        } else if (e2_min >= e1_min && e2_min <= e1_max) {
            min = e2_min;
        } else {
            break;
        }
        T max = std::min(e1_max, e2_max);
        if (!((previous_min >= min && previous_min <= max) ||
              (previous_max >= min && previous_max <= max))) {
            break;
        }
        previous_max = max;
        previous_min = min;
        pt.x = min + (max - min) / 2;
        pt.y = current_y;
        bool hot_pixel_found = false;
        while (hot_pixel_itr != hot_pixels.end() && hot_pixel_itr->y == current_y) {
            if (hot_pixel_itr->x >= min && hot_pixel_itr->x <= max) {
                if (hot_pixel_found) {
                    // Is this pixel closer to the middle then previous
                    // hot pixel
                    double middle = static_cast<double>(min) + (max - min) / 2.0;
                    double prev_dist_to_middle = std::fabs(middle - pt.x);
                    double dist_to_middle = std::fabs(middle - hot_pixel_itr->x);
                    if (prev_dist_to_middle > dist_to_middle) {
                        pt.x = hot_pixel_itr->x;
                    }
                } else {
                    pt.x = hot_pixel_itr->x;
                    hot_pixel_found = true;
                }
            }
            ++hot_pixel_itr;
        }
        if (hot_pixel_found) {
            break;
        }
        ++current_y;
    }
}

template <typename T>
void intersection_point(bound<T> const& Bound1,
                        bound<T> const& Bound2,
                        mapbox::geometry::point<T>& ip,
                        hot_pixel_set<T>& hot_pixels,
                        mapbox::geometry::point<double> const& int_pt) {
    // This method finds the FIRST intersecting point in integer space between
    // two edges that is closest to the bot point of the edges.
    edge<T> const& Edge1 = *Bound1.current_edge;
    edge<T> const& Edge2 = *Bound2.current_edge;

    // Bottom of scanbeam!
    T bottom_y = std::max(std::llround(Bound1.curr.y), std::llround(Bound2.curr.y));

    if (std::fabs(Edge1.dx - Edge2.dx) < std::numeric_limits<double>::epsilon()) {
        ip.y = std::llround(Bound1.curr.y);
        ip.x = std::llround(get_current_x(Edge1, ip.y));
        return;
    } else if (std::fabs(Edge1.dx) < std::numeric_limits<double>::epsilon()) {
        ip.x = Edge1.bot.x;
        if (is_horizontal(Edge2)) {
            ip.y = Edge2.bot.y;
        } else {
            double b2 = static_cast<double>(Edge2.bot.y) - (Edge2.bot.x / Edge2.dx);
            if (Edge2.bot.x == Edge1.bot.x) {
                ip.y = std::llround(static_cast<double>(ip.x) / Edge2.dx + b2);
            } else if (Edge2.bot.x < Edge1.bot.x) {
                ip.y = std::llround((static_cast<double>(ip.x) - 0.5) / Edge2.dx + b2);
            } else {
                ip.y = std::llround((static_cast<double>(ip.x) + 0.5) / Edge2.dx + b2);
            }
        }
    } else if (std::fabs(Edge2.dx) < std::numeric_limits<double>::epsilon()) {
        ip.x = Edge2.bot.x;
        if (is_horizontal(Edge1)) {
            ip.y = Edge1.bot.y;
        } else {
            double b1 = static_cast<double>(Edge1.bot.y) - (Edge1.bot.x / Edge1.dx);
            if (Edge1.bot.x == Edge2.bot.x) {
                ip.y = std::llround(static_cast<double>(ip.x) / Edge1.dx + b1);
            } else if (Edge1.bot.x < Edge2.bot.x) {
                ip.y = std::llround((static_cast<double>(ip.x) - 0.5) / Edge1.dx + b1);
            } else {
                ip.y = std::llround((static_cast<double>(ip.x) + 0.5) / Edge1.dx + b1);
            }
        }
    } else {
        if (Edge1.dx > 0.0 && Edge2.dx > 0.0) {
            find_nearest_shared_point_right(Edge1, Edge2, ip, hot_pixels, bottom_y, int_pt);
        } else if (Edge1.dx < 0.0 && Edge2.dx < 0.0) {
            find_nearest_shared_point_left(Edge1, Edge2, ip, hot_pixels, bottom_y, int_pt);
        } else {
            find_nearest_shared_point_center(Edge1, Edge2, ip, hot_pixels, bottom_y, int_pt);
        }
    }

    if (ip.y < Edge1.top.y || ip.y < Edge2.top.y) {
        if (Edge1.top.y > Edge2.top.y) {
            ip.y = Edge1.top.y;
        } else {
            ip.y = Edge2.top.y;
        }
        if (std::fabs(Edge1.dx) < std::fabs(Edge2.dx)) {
            ip.x = std::llround(get_current_x(Edge1, ip.y));
        } else {
            ip.x = std::llround(get_current_x(Edge2, ip.y));
        }
    }
    // finally, don't allow 'ip' to be BELOW curr.y (ie bottom of scanbeam) ...
    if (ip.y > std::llround(Bound1.curr.y)) {
        ip.y = std::llround(Bound1.curr.y);
        // use the more vertical edge to derive X ...
        if (std::fabs(Edge1.dx) > std::fabs(Edge2.dx)) {
            ip.x = std::llround(get_current_x(Edge2, ip.y));
        } else {
            ip.x = std::llround(get_current_x(Edge1, ip.y));
        }
    }
}
}
}
}
