#pragma once

#include <list>

#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/box.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/vatti.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
class clipper {
private:
    using value_type = T;

    local_minimum_list<value_type> minima_list;
    std::vector<edge_list<value_type>> m_edges;
    bool allow_collinear;
    bool has_open_paths;
    bool reverse_output_rings;

public:
    clipper()
        : minima_list(),
          m_edges(),
          allow_collinear(false),
          has_open_paths(false),
          reverse_output_rings(false) {
    }

    ~clipper() {
        clear();
    }

    bool add_line(mapbox::geometry::line_string<value_type> const& pg) {
        bool success = add_line_string(pg, m_edges, minima_list);
        if (success) {
            has_open_paths = true;
        }
        return success;
    }

    bool add_ring(mapbox::geometry::linear_ring<value_type> const& pg,
                  polygon_type p_type = polygon_type_subject) {
        return add_linear_ring(pg, m_edges, minima_list, p_type);
    }

    bool add_polygon(mapbox::geometry::polygon<value_type> const& ppg,
                     polygon_type p_type = polygon_type_subject) {
        bool result = false;
        for (std::size_t i = 0; i < ppg.size(); ++i) {
            if (add_ring(ppg[i], p_type)) {
                result = true;
            }
        }
        return result;
    }

    void clear() {
        minima_list.clear();
        m_edges.clear();
        has_open_paths = false;
    }

    box<value_type> get_bounds() {
        box<value_type> result = { 0, 0, 0, 0 };
        auto lm = minima_list.begin();
        if (lm == minima_list.end()) {
            return result;
        }
        result.left = lm->left_bound->bot.x;
        result.top = lm->left_bound->bot.y;
        result.right = lm->left_bound->bot.x;
        result.bottom = lm->left_bound->bot.y;
        while (lm != minima_list.end()) {
            // todo - needs fixing for open paths
            result.bottom = std::max(result.bottom, lm->left_bound->bot.y);
            edge_ptr<value_type> e = lm->left_bound;
            for (;;) {
                edge_ptr<value_type> bottomE = e;
                while (e->next_in_LML) {
                    if (e->bot.x < result.left) {
                        result.left = e->bot.x;
                    }
                    if (e->bot.x > result.right) {
                        result.right = e->bot.x;
                    }
                    e = e->next_in_LML;
                }
                result.left = std::min(result.left, e->bot.x);
                result.right = std::max(result.right, e->bot.x);
                result.left = std::min(result.left, e->top.x);
                result.right = std::max(result.right, e->top.x);
                result.top = std::min(result.top, e->top.y);
                if (bottomE == lm->left_bound) {
                    e = lm->right_bound;
                } else {
                    break;
                }
            }
            ++lm;
        }
        return result;
    }

    bool preserve_collinear() {
        return allow_collinear;
    }

    void preserve_collinear(bool value) {
        allow_collinear = value;
    }

    bool reverse_output() {
        return reverse_output_rings;
    }

    void reverse_output(bool value) {
        reverse_output_rings = value;
    }

    bool execute(clip_type cliptype,
                 std::vector<mapbox::geometry::polygon<value_type>>& solution,
                 fill_type subject_fill_type,
                 fill_type clip_fill_type) {
        solution.clear(); // put here to do nothing for now.
        ring_list<T> rings;

        bool worked =
            execute_vatti(minima_list, rings, cliptype, subject_fill_type, clip_fill_type);

        build_result(solution, rings);

        for (auto& r : rings) {
            dispose_out_points(r->points);
            delete r;
        }

        return worked;
    }

    void build_result(std::vector<mapbox::geometry::polygon<value_type>>& solution,
                      ring_list<value_type>& rings) {

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

    void push_ring_to_polygon(mapbox::geometry::polygon<value_type>& poly, ring_ptr<T>& r) {
        mapbox::geometry::linear_ring<value_type> lr;
        auto firstPt = r->points;
        auto ptIt = r->points;
        do {
            lr.push_back({ ptIt->x, ptIt->y });
            ptIt = ptIt->next;
        } while (ptIt != firstPt);
        lr.push_back({ firstPt->x, firstPt->y }); // close the ring
        poly.push_back(lr);
    }
};
}
}
}
