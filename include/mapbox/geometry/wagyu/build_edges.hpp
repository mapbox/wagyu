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
bool build_edge_list(mapbox::geometry::linear_ring<T> const& path_geometry, edge_list<T>& edges) {
    using value_type = T;

    if (path_geometry.size() < 3) {
        return false;
    }

    // As this is a loop, we need to first go backwards from end to try and find
    // the proper starting point for the iterators before the beginning

    auto itr_rev = path_geometry.rbegin();
    auto itr = path_geometry.begin();
    mapbox::geometry::point<value_type> pt1 = *itr_rev;
    mapbox::geometry::point<value_type> pt2 = *itr;

    // Find next non repeated point going backwards from
    // end for pt1
    while (pt1 == pt2) {
        pt1 = *(++itr_rev);
        if (itr_rev == path_geometry.rend()) {
            return false;
        }
    }
    ++itr;
    mapbox::geometry::point<value_type> pt3 = *itr;
    auto itr_last = itr_rev.base();
    mapbox::geometry::point<value_type> front_pt;
    mapbox::geometry::point<value_type> back_pt;
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
            // We need to reconsider previously added points
            // because the point it was using was found to be collinear
            // or a spike
            pt2 = pt1;
            if (!edges.empty()) {
                edges.pop_back(); // remove previous edge (pt1)
            }
            if (!edges.empty()) {
                if (back_pt == edges.back().top) {
                    pt1 = edges.back().bot;
                } else {
                    pt1 = edges.back().top;
                }
                back_pt = pt1;
            } else {
                // If this occurs we must look to the back of the
                // ring for new points.
                do {
                    ++itr_rev;
                    if ((itr + 1) == itr_rev.base()) {
                        return false;
                    }
                    pt1 = *itr_rev;
                } while (pt1 == pt2);
                itr_last = itr_rev.base();
            }
            continue;
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

    if (edges.size() < 3) {
        return false;
    }
    auto& f = edges.front();
    auto& b = edges.back();
    if (slopes_equal(f, b)) {
        if (f.bot == b.top) {
            f.bot = b.bot;
            edges.pop_back();
        } else if (f.top == b.bot) {
            f.top = b.top;
            edges.pop_back();
        }
    }
    if (edges.size() < 3) {
        return false;
    }
    return true;
}
}
}
}
