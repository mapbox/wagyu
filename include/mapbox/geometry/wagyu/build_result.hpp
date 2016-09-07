#pragma once

#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

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

template <typename T> 
void build_result_polygons(std::vector<mapbox::geometry::polygon<T>>& solution, ring_list<T>& rings) {
    
    for (auto& r : rings) {
        assert(r->points);
        std::size_t cnt = point_count(r->points);
        if ((r->is_open && cnt < 2) || (!r->is_open && cnt < 3)) {
            continue;
        }
        solution.emplace_back();
        push_ring_to_polygon(solution.back(), r);
        for (auto & c : r->children) {
            assert(c->points);
            cnt = point_count(c->points);
            if ((c->is_open && cnt < 2) || (!c->is_open && cnt < 3)) {
                continue;
            }
            push_ring_to_polygon(solution.back(), c);
        }
        for (auto & c : r->children) {
            if (!c->children.empty()) {
                build_result_polygons(solution, c->children);
            }
        }
    }
}

template <typename T>
void build_result(std::vector<mapbox::geometry::polygon<T>>& solution, ring_manager<T>& rings) {
    build_result_polygons(solution, rings.children);
}

}
}
}
