#pragma once

#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
bool build_edge_list(mapbox::geometry::line_string<T> const& path_geometry,
                     edge_list<T>& edges,
                     bool& is_flat) {
    if (path_geometry.size() < 2) {
        return false;
    }

    auto itr_next = path_geometry.begin();
    ++itr_next;
    auto itr = path_geometry.begin();
    while (itr_next != path_geometry.end()) {
        if (*itr_next == *itr) {
            // Duplicate point advance itr_next, but do not
            // advance itr
            ++itr_next;
            continue;
        }

        if (is_flat && itr_next->y != itr->y) {
            is_flat = false;
        }
        edges.emplace_back(*itr, *itr_next);
        itr = itr_next;
        ++itr_next;
    }

    if (edges.size() < 2) {
        return false;
    }

    return true;
}

template <typename T>
bool point_2_is_between_point_1_and_point_3(mapbox::geometry::point<T> const& pt1,
                                            mapbox::geometry::point<T> const& pt2,
                                            mapbox::geometry::point<T> const& pt3) {
    if ((pt1 == pt3) || (pt1 == pt2) || (pt3 == pt2)) {
        return false;
    } else if (pt1.x != pt3.x) {
        return (pt2.x > pt1.x) == (pt2.x < pt3.x);
    } else {
        return (pt2.y > pt1.y) == (pt2.y < pt3.y);
    }
}

template <typename T>
mapbox::geometry::linear_ring<T> remove_collinear(mapbox::geometry::linear_ring<T> path_geometry) {
    mapbox::geometry::linear_ring<T> out;
    size_t s = path_geometry.size();

    // Starts at 1 because of test expectation that point 0 will stay in first position
    if (s > 0) {
        out.push_back(path_geometry[0]);
    }
    for (size_t i = 1; i < path_geometry.size(); i++) {
        if (!slopes_equal(path_geometry[(i + s - 1) % s], path_geometry[i],
                          path_geometry[(i + 1) % s])) {
            out.push_back(path_geometry[i]);
        }
    }

    return out;
}

template <typename T>
bool build_edge_list(mapbox::geometry::linear_ring<T> path_geometry, edge_list<T>& edges) {
    path_geometry = remove_collinear(path_geometry);

    if (path_geometry.size() < 3) {
        return false;
    }

    // As this is a loop, we need to first go backwards from end to try and find
    // the proper starting point for the iterators before the beginning

    auto itr_rev = path_geometry.rbegin();
    auto itr = path_geometry.begin();
    mapbox::geometry::point<T> pt1 = *itr_rev;
    mapbox::geometry::point<T> pt2 = *itr;

    // Find next non repeated point going backwards from
    // end for pt1
    while (pt1 == pt2) {
        ++itr_rev;
        if (itr_rev == path_geometry.rend()) {
            return false;
        }
        pt1 = *itr_rev;
    }
    ++itr;
    mapbox::geometry::point<T> pt3 = *itr;
    auto itr_last = itr_rev.base();
    mapbox::geometry::point<T> front_pt;
    mapbox::geometry::point<T> back_pt;
    while (true) {
        if (pt3 == pt2) {
            // Duplicate point advance itr, but do not
            // advance other points
            if (itr == itr_last) {
                break;
            }
            ++itr;
            if (itr == itr_last) {
                if (edges.empty()) {
                    break;
                }
                pt3 = front_pt;
            } else {
                pt3 = *itr;
            }
            continue;
        }

        // Now check if slopes are equal between two segments - either
        // a spike or a collinear point - if so drop point number 2.
        if (slopes_equal(pt1, pt2, pt3)) {
            assert("Can't happen: collinear points");
        }

        if (edges.empty()) {
            front_pt = pt2;
        }
        edges.emplace_back(pt2, pt3);
        back_pt = pt2;
        if (itr == itr_last) {
            break;
        }
        pt1 = pt2;
        pt2 = pt3;
        ++itr;
        if (itr == itr_last) {
            if (edges.empty()) {
                break;
            }
            pt3 = front_pt;
        } else {
            pt3 = *itr;
        }
    }

    bool modified = false;
    do {
        modified = false;
        if (edges.size() < 3) {
            return false;
        }
        auto& f = edges.front();
        auto& b = edges.back();
        if (slopes_equal(f, b)) {
            if (f.bot == b.top) {
                if (f.top == b.bot) {
                    edges.pop_back();
                    edges.erase(edges.begin());
                } else {
                    f.bot = b.bot;
                    edges.pop_back();
                }
                modified = true;
            } else if (f.top == b.bot) {
                f.top = b.top;
                edges.pop_back();
                modified = true;
            } else if (f.top == b.top && f.bot == b.bot) {
                edges.pop_back();
                edges.erase(edges.begin());
                modified = true;
            } else if (f.top == b.top) {
                if (point_2_is_between_point_1_and_point_3(f.top, f.bot, b.bot)) {
                    b.top = f.bot;
                    edges.erase(edges.begin());
                } else {
                    f.top = b.bot;
                    edges.pop_back();
                }
                modified = true;
            } else if (f.bot == b.bot) {
                if (point_2_is_between_point_1_and_point_3(f.bot, f.top, b.top)) {
                    b.bot = f.top;
                    edges.erase(edges.begin());
                } else {
                    f.bot = b.top;
                    edges.pop_back();
                }
                modified = true;
            }
        }
    } while (modified);

    return true;
}
}
}
}
