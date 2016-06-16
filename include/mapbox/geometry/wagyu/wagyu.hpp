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
                 mapbox::geometry::polygon<value_type>& solution,
                 fill_type subject_fill_type,
                 fill_type clip_fill_type) {
        solution.clear(); // put here to do nothing for now.
        ring_list<T> rings;
        return execute_vatti(minima_list, rings, cliptype, subject_fill_type, clip_fill_type);
    }
};
}
}
}
