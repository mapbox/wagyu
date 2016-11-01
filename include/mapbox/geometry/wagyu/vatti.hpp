#pragma once

#include <algorithm>
#include <set>

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/intersect_util.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/local_minimum_util.hpp>
#include <mapbox/geometry/wagyu/process_horizontal.hpp>
#include <mapbox/geometry/wagyu/process_maxima.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void update_hotpixels_to_scanline(T scanline_y,
                                  active_bound_list<T>& active_bounds,
                                  ring_manager<T>& rings) {
    for (auto & bnd : active_bounds) {
        mapbox::geometry::point<T> scanline_point(
            std::llround(get_current_x(*(bnd->current_edge), scanline_y)), scanline_y);
        insert_hot_pixels_in_path(*bnd, scanline_point, rings, true);
    }
    
    while(rings.current_hp_itr != rings.hot_pixels.end()) {
        if (rings.current_hp_itr->y >= scanline_y) {
            break;
        }
        ++rings.current_hp_itr;
    }
}

template <typename T>
void clear_hot_pixels(T scanline_y, ring_manager<T>& rings) {

    auto end = std::remove_if(rings.hot_pixels.begin(), rings.hot_pixels.end(),
                                  [scanline_y](const mapbox::geometry::point<T> & pt)
                                   { return pt.y != scanline_y; });
    rings.hot_pixels.erase(end, rings.hot_pixels.end());
}

template <typename T>
bool execute_vatti(local_minimum_list<T>& minima_list,
                   ring_manager<T>& rings,
                   clip_type cliptype,
                   fill_type subject_fill_type,
                   fill_type clip_fill_type) {

    if (minima_list.empty()) {
        return false;
    }
    
    active_bound_list<T> active_bounds;
    scanbeam_list<T> scanbeam;
    T scanline_y = std::numeric_limits<T>::max();

    local_minimum_ptr_list<T> minima_sorted;
    minima_sorted.reserve(minima_list.size());
    for (auto& lm : minima_list) {
        minima_sorted.push_back(&lm);
    }
    std::stable_sort(minima_sorted.begin(), minima_sorted.end(), local_minimum_sorter<T>());
    local_minimum_ptr_list_itr<T> current_lm = minima_sorted.begin();
    // std::clog << output_all_edges(minima_sorted) << std::endl;

    setup_scanbeam(minima_list, scanbeam);
    rings.current_hp_itr = rings.hot_pixels.begin();
    preallocate_point_memory(rings, rings.hot_pixels.size());

    while (pop_from_scanbeam(scanline_y, scanbeam) || current_lm != minima_sorted.end()) {

        process_intersections(scanline_y, active_bounds, cliptype,
                              subject_fill_type, clip_fill_type, rings);

        // First we process bounds that has already been added to the active bound list --
        // if the active bound list is empty local minima that are at this scanline_y and
        // have a horizontal edge at the local minima will be processed
        process_edges_at_top_of_scanbeam(scanline_y, active_bounds, scanbeam, minima_sorted,
                                         current_lm, rings, cliptype, subject_fill_type,
                                         clip_fill_type);

        // Next we will add local minima bounds to the active bounds list that are on the local
        // minima queue at
        // this current scanline_y
        insert_local_minima_into_ABL(scanline_y, minima_sorted, current_lm, active_bounds,
                                     rings, scanbeam, cliptype, subject_fill_type,
                                     clip_fill_type);

        update_hotpixels_to_scanline(scanline_y, active_bounds, rings);
    }
    // std::clog << rings.all_rings << std::endl;
    // std::clog << output_as_polygon(rings.all_rings[0]);
    return true;
}
}
}
}
