#pragma once

#include <algorithm>
#include <set>

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/build_edges.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/intersect_util.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/local_minimum_util.hpp>
#include <mapbox/geometry/wagyu/process_horizontal.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/topology_correction.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
bool add_line_string(mapbox::geometry::line_string<T> const& path_geometry,
                     local_minimum_list<T>& minima_list) {
    bool is_flat = true;
    edge_list<T> new_edges;
    if (!build_edge_list(path_geometry, new_edges, is_flat) || new_edges.empty()) {
        return false;
    }
    add_line_to_local_minima_list(new_edges, minima_list, polygon_type_subject);
    return true;
}

template <typename T>
bool add_linear_ring(mapbox::geometry::linear_ring<T> const& path_geometry,
                     local_minimum_list<T>& minima_list,
                     polygon_type p_type) {
    edge_list<T> new_edges;
    if (!build_edge_list(path_geometry, new_edges) || new_edges.empty()) {
        return false;
    }
    add_ring_to_local_minima_list(new_edges, minima_list, p_type);
    return true;
}

template <typename T>
active_bound_list_itr<T> do_maxima(active_bound_list_itr<T>& bnd,
                                   active_bound_list_itr<T>& bndMaxPair,
                                   clip_type cliptype,
                                   fill_type subject_fill_type,
                                   fill_type clip_fill_type,
                                   ring_manager<T>& rings,
                                   active_bound_list<T>& active_bounds) {
    if (bndMaxPair == active_bounds.end()) {
        if ((*bnd)->ring) {
            add_point_to_ring(*(*bnd), (*bnd)->current_edge->top, rings);
        }
        return active_bounds.erase(bnd);
    }
    auto bnd_next = std::next(bnd);
    auto return_bnd = bnd_next;
    bool skipped = false;
    while (bnd_next != active_bounds.end() && bnd_next != bndMaxPair) {
        skipped = true;
        intersect_bounds(bnd, bnd_next, (*bnd)->current_edge->top, cliptype, subject_fill_type,
                         clip_fill_type, rings, active_bounds);
        swap_positions_in_ABL(bnd, bnd_next, active_bounds);
        bnd_next = std::next(bnd);
    }

    if (!(*bnd)->ring && !(*bndMaxPair)->ring) {
        active_bounds.erase(bndMaxPair);
    } else if ((*bnd)->ring && (*bndMaxPair)->ring) {
        add_local_maximum_point(bnd, bndMaxPair, (*bnd)->current_edge->top, rings, active_bounds);
        active_bounds.erase(bndMaxPair);
    } else if ((*bnd)->winding_delta == 0 && (*bnd)->ring) {
        add_point_to_ring(*(*bnd), (*bnd)->current_edge->top, rings);
        active_bounds.erase(bndMaxPair);
    } else if ((*bnd)->winding_delta == 0 && (*bndMaxPair)->ring) {
        add_point_to_ring(*(*bndMaxPair), (*bnd)->current_edge->top, rings);
        active_bounds.erase(bndMaxPair);
    } else {
        throw clipper_exception("DoMaxima error");
    }
    auto prev_itr = active_bounds.erase(bnd);
    if (skipped) {
        return return_bnd;
    } else {
        return prev_itr;
    }
}

template <typename T>
void process_edges_at_top_of_scanbeam(T top_y,
                                      active_bound_list<T>& active_bounds,
                                      scanbeam_list<T>& scanbeam,
                                      local_minimum_ptr_list<T> const& minima_sorted,
                                      local_minimum_ptr_list_itr<T>& current_lm,
                                      ring_manager<T>& rings,
                                      clip_type cliptype,
                                      fill_type subject_fill_type,
                                      fill_type clip_fill_type) {

    for (auto bnd = active_bounds.begin(); bnd != active_bounds.end();) {
        // 1. Process maxima, treating them as if they are "bent" horizontal edges,
        // but exclude maxima with horizontal edges.

        bool is_maxima_edge = is_maxima(bnd, top_y);

        active_bound_list_itr<T> bnd_max_pair;
        if (is_maxima_edge) {
            bnd_max_pair = get_maxima_pair(bnd, active_bounds);
            is_maxima_edge = ((bnd_max_pair == active_bounds.end() ||
                               !current_edge_is_horizontal<T>(bnd_max_pair)) &&
                              is_maxima(bnd_max_pair, top_y));
        }

        if (is_maxima_edge) {
            bnd = do_maxima(bnd, bnd_max_pair, cliptype, subject_fill_type, clip_fill_type, rings,
                            active_bounds);
        } else {
            // 2. Promote horizontal edges.
            if (is_intermediate(bnd, top_y) && next_edge_is_horizontal<T>(bnd)) {
                insert_hot_pixels_in_path(*(*bnd), (*bnd)->current_edge->top, rings, false);
                next_edge_in_bound(bnd, scanbeam);
                if ((*bnd)->ring) {
                    add_point_to_ring(*(*bnd), (*bnd)->current_edge->bot, rings);
                    mapbox::geometry::point<T> hp((*bnd)->current_edge->top.x, top_y);
                    add_to_hot_pixels(hp, rings);
                }
            } else {
                (*bnd)->curr.x = get_current_x(*((*bnd)->current_edge), top_y);
                (*bnd)->curr.y = static_cast<double>(top_y);
            }

            ++bnd;
        }
    }

    insert_horizontal_local_minima_into_ABL(top_y, minima_sorted, current_lm, active_bounds, rings,
                                            scanbeam, cliptype, subject_fill_type, clip_fill_type);

    process_horizontals(top_y, active_bounds, rings, scanbeam, cliptype, subject_fill_type,
                        clip_fill_type);

    // 4. Promote intermediate vertices

    for (auto bnd = active_bounds.begin(); bnd != active_bounds.end(); ++bnd) {
        if (is_intermediate(bnd, top_y)) {
            if ((*bnd)->ring) {
                add_point_to_ring(*(*bnd), (*bnd)->current_edge->top, rings);
            }
            insert_hot_pixels_in_path(*(*bnd), (*bnd)->current_edge->top, rings, false);
            next_edge_in_bound(bnd, scanbeam);
        }
    }
}

template <typename T>
void fixup_out_polyline(ring<T>& ring, ring_manager<T>& rings) {
    point_ptr<T> pp = ring.points;
    point_ptr<T> lastPP = pp->prev;
    while (pp != lastPP) {
        pp = pp->next;
        if (*pp == *pp->prev) {
            if (pp == lastPP)
                lastPP = pp->prev;
            point_ptr<T> tmpPP = pp->prev;
            tmpPP->next = pp->next;
            pp->next->prev = tmpPP;
            // delete pp;
            pp->next = pp;
            pp->prev = pp;
            pp->ring = nullptr;
            pp = tmpPP;
        }
    }

    if (pp == pp->prev) {
        remove_ring(&ring, rings);
        dispose_out_points(pp);
        ring.points = nullptr;
        return;
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

template <typename T>
void fixup_out_polygon(ring<T>& ring, ring_manager<T>& rings, bool simple) {
    // FixupOutPolygon() - removes duplicate points and simplifies consecutive
    // parallel edges by removing the middle vertex.
    point_ptr<T> lastOK = nullptr;
    ring.bottom_point = nullptr;
    point_ptr<T> pp = ring.points;

    for (;;) {
        if (pp->prev == pp || pp->prev == pp->next) {
            // We now need to make sure any children rings to this are promoted and their hole
            // status is changed
            // promote_children_of_removed_ring(&ring, rings);
            remove_ring(&ring, rings);
            dispose_out_points(pp);
            ring.points = nullptr;
            return;
        }

        // test for duplicate points and collinear edges ...
        if ((*pp == *pp->next) || (*pp == *pp->prev) ||
            (slopes_equal(*pp->prev, *pp, *pp->next) &&
             (!simple || !point_2_is_between_point_1_and_point_3(*pp->prev, *pp, *pp->next)))) {
            lastOK = nullptr;
            point_ptr<T> tmp = pp;
            pp->prev->next = pp->next;
            pp->next->prev = pp->prev;
            pp = pp->prev;
            tmp->ring = nullptr;
            tmp->next = tmp;
            tmp->prev = tmp;
        } else if (pp == lastOK) {
            break;
        } else {
            if (!lastOK) {
                lastOK = pp;
            }
            pp = pp->next;
        }
    }
    ring.points = pp;
}

template <typename T>
void remove_spikes_in_polygons(ring_ptr<T> r, ring_manager<T>& rings) {

    point_ptr<T> first_point = r->points;
    remove_spikes(first_point);
    if (!first_point) {
        r->points = nullptr;
        r->area = std::numeric_limits<double>::quiet_NaN();
        remove_ring(r, rings);
        return;
    }
    point_ptr<T> p = first_point->next;
    while (p != first_point) {
        remove_spikes(p);
        if (!p) {
            r->points = nullptr;
            r->area = std::numeric_limits<double>::quiet_NaN();
            remove_ring(r, rings);
            return;
        }
        if (p->ring && !first_point->ring) {
            first_point = p;
        }
        p = p->next;
    }
}

template <typename T>
void update_hotpixels_to_scanline(T scanline_y,
                                  active_bound_list<T>& active_bounds,
                                  ring_manager<T>& rings) {
    for (auto bnd : active_bounds) {
        mapbox::geometry::point<T> scanline_point(
            std::llround(get_current_x(*(bnd->current_edge), scanline_y)), scanline_y);
        insert_hot_pixels_in_path(*bnd, scanline_point, rings, true);
    }
}

template <typename T>
void clear_hot_pixels(T scanline_y, ring_manager<T>& rings) {
    for (auto itr = rings.hot_pixels.begin(); itr != rings.hot_pixels.end();) {
        if (itr->first != scanline_y) {
            itr = rings.hot_pixels.erase(itr);
        } else {
            ++itr;
        }
    }
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

    {
        // This section in its own { } to limit memory scope of variables
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

        while (pop_from_scanbeam(scanline_y, scanbeam) || current_lm != minima_sorted.end()) {

            process_intersections(scanline_y, minima_sorted, current_lm, active_bounds, cliptype,
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

            clear_hot_pixels(scanline_y, rings);
        }
    }

    // std::clog << rings.all_rings << std::endl;
    // std::clog << output_as_polygon(rings.all_rings[0]);

    // fix orientations ...
    for (auto& r : rings.all_rings) {
        if (!r->points || r->is_open) {
            continue;
        }
        if (ring_is_hole(r) == (area_from_point(r->points) > 0)) {
            reverse_ring(r->points);
        }
        remove_spikes_in_polygons(r, rings);
        r->area = std::numeric_limits<double>::quiet_NaN();
    }

    do_simple_polygons(rings);

#if DEBUG
    for (auto& r : rings.all_rings) {
        if (!r->points || r->is_open) {
            continue;
        }
        double stored_area = area(r);
        double calculated_area = area_from_point(r->points);
        if (values_near_equal(stored_area, calculated_area)) {
            throw std::runtime_error("Difference in stored area vs calculated area!");
        }
    }
#endif

    for (auto& r : rings.all_rings) {
        if (!r->points || r->is_open) {
            continue;
        }
        fixup_out_polygon(*r, rings, false);
        if (ring_is_hole(r) == (area(r) > 0.0)) {
            reverse_ring(r->points);
            r->area = std::numeric_limits<double>::quiet_NaN();
        }
    }

    return true;
}
}
}
}
