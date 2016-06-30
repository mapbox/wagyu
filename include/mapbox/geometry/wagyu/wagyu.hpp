#pragma once

#include <list>

#include <mapbox/geometry/box.hpp>
#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/vatti.hpp>
#include <mapbox/geometry/wagyu/build_result.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
class clipper {
private:
    using value_type = T;

    local_minimum_list<value_type> minima_list;
    bool has_open_paths;

public:
    clipper()
        : minima_list(),
          has_open_paths(false) {
    }

    ~clipper() {
        clear();
    }

    bool add_line(mapbox::geometry::line_string<value_type> const& pg) {
        bool success = add_line_string(pg, minima_list);
        if (success) {
            has_open_paths = true;
        }
        return success;
    }

    bool add_ring(mapbox::geometry::linear_ring<value_type> const& pg,
                  polygon_type p_type = polygon_type_subject) {
        return add_linear_ring(pg, minima_list, p_type);
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
        has_open_paths = false;
    }

    mapbox::geometry::box<value_type> get_bounds() {
        mapbox::geometry::point<value_type> min = {0,0};
        mapbox::geometry::point<value_type> max = {0,0};
        if (minima_list.empty()) {
            return mapbox::geometry::box<value_type>(min,max);
        }
        bool first_set = false;
        for (auto const& lm : minima_list) {
            if (!lm.left_bound.edges.empty()) {
                if (!first_set) {
                    min = lm.left_bound.edges.front().top;
                    max = lm.left_bound.edges.back().bot;
                    first_set = true;
                } else {
                    min.y = std::min(min.y, lm.left_bound.edges.front().top.y);
                    max.y = std::max(max.y, lm.left_bound.edges.back().bot.y);
                    max.x = std::max(max.x, lm.left_bound.edges.back().top.x);
                    min.x = std::min(min.x, lm.left_bound.edges.back().top.x);
                }
                for (auto const& e : lm.left_bound.edges) {
                    max.x = std::max(max.x, e.bot.x);
                    min.x = std::min(min.x, e.bot.x);
                }
            }
            if (!lm.right_bound.edges.empty()) {
                if (!first_set) {
                    min = lm.right_bound.edges.front().top;
                    max = lm.right_bound.edges.back().bot;
                    first_set = true;
                } else {
                    min.y = std::min(min.y, lm.right_bound.edges.front().top.y);
                    max.y = std::max(max.y, lm.right_bound.edges.back().bot.y);
                    max.x = std::max(max.x, lm.right_bound.edges.back().top.x);
                    min.x = std::min(min.x, lm.right_bound.edges.back().top.x);
                }
                for (auto const& e : lm.right_bound.edges) {
                    max.x = std::max(max.x, e.bot.x);
                    min.x = std::min(min.x, e.bot.x);
                }
            }
        }
        return mapbox::geometry::box<value_type>(min,max);
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

};
}
}
}
