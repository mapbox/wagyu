#pragma once

#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void build_result(std::vector<mapbox::geometry::polygon<T>>& solution,
                  ring_list<T>& rings) {

    // loop through constructing polygons
    for (auto& r : rings) {
        if (!r->points || r->ring_index > 0) {
            continue;
        }
        std::size_t cnt = point_count(r->points);
        if ((r->is_open && cnt < 2) || (!r->is_open && cnt < 3)) {
            continue;
        }
        if (r->is_hole) {
            // create the parent ring polygon first
            auto fl = parse_first_left(r->first_left);
            if (!fl->ring_index) {
                solution.emplace_back();
                fl->ring_index = solution.size();
                push_ring_to_polygon(solution.back(), fl);
            }
            push_ring_to_polygon(solution[fl->ring_index - 1], r);
        } else {
            solution.emplace_back();
            push_ring_to_polygon(solution.back(), r);
        }
    }
}

template <typename T>
void push_ring_to_polygon(mapbox::geometry::polygon<T>& poly, ring_ptr<T>& r) {
    mapbox::geometry::linear_ring<T> lr;
    auto firstPt = r->points;
    auto ptIt = r->points;
    do {
        lr.push_back({ ptIt->x, ptIt->y });
        ptIt = ptIt->next;
    } while (ptIt != firstPt);
    lr.push_back({ firstPt->x, firstPt->y }); // close the ring
    poly.push_back(lr);
}

}
}
}
