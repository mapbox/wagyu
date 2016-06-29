#pragma once

#include <algorithm>
#include <set>
#include <unordered_map>

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/build_edges.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/intersect_util.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/join_util.hpp>
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
                     std::vector<edge_list<T>>& edges,
                     local_minimum_list<T>& minima_list) {
    bool is_flat = true;
    edges.emplace_back();
    auto& new_edges = edges.back();
    if (!build_edge_list(path_geometry, new_edges, is_flat) || new_edges.empty()) {
        edges.pop_back();
        return false;
    }
    add_line_to_local_minima_list(new_edges, minima_list, polygon_type_subject);
    return true;
}

template <typename T>
bool add_linear_ring(mapbox::geometry::linear_ring<T> const& path_geometry,
                     std::vector<edge_list<T>>& edges,
                     local_minimum_list<T>& minima_list,
                     polygon_type p_type) {
    edges.emplace_back();
    auto& new_edges = edges.back();
    if (!build_edge_list(path_geometry, new_edges) || new_edges.empty()) {
        edges.pop_back();
        return false;
    }
    add_ring_to_local_minima_list(new_edges, minima_list, p_type);
    return true;
}

template <typename T>
active_bound_list_itr<T> do_maxima(active_bound_list_itr<T> bnd,
                                   active_bound_list_itr<T> bndMaxPair,
                                   clip_type cliptype,
                                   fill_type subject_fill_type,
                                   fill_type clip_fill_type,
                                   ring_list<T>& rings,
                                   join_list<T>& joins,
                                   active_bound_list<T>& active_bounds) {
    if (bndMaxPair == active_bounds.end()) {
        if ((*bnd)->ring) {
            add_point_to_ring(bnd, (*bnd)->current_edge->top);
        }
        return active_bounds.erase(bnd);
    }

    auto bnd_prev = active_bound_list_rev_itr<T>(bnd);
    if (bnd_prev != active_bounds.rend() && (*bnd_prev)->curr.x == (*bnd)->current_edge->top.x &&
        (*bnd_prev)->current_edge->top != (*bnd)->current_edge->top && (*bnd_prev)->ring &&
        (*bnd_prev)->winding_delta != 0 && (*bnd)->ring && (*bnd)->winding_delta != 0) {
        add_point_to_ring(bnd_prev, (*bnd)->current_edge->top);
    }
    auto bnd_next = std::next(bnd);
    while (bnd_next != active_bounds.end() && bnd_next != bndMaxPair) {
        intersect_bounds(bnd, bnd_next, (*bnd)->current_edge->top, cliptype, subject_fill_type,
                         clip_fill_type, rings, joins, active_bounds);
        active_bounds.splice(bnd, active_bounds, bnd_next);
        bnd_next = std::next(bnd);
    }
    bnd_next = std::next(bndMaxPair);
    if (bnd_next != active_bounds.end() && (*bnd_next)->curr.x == (*bnd)->current_edge->top.x &&
        (*bnd_next)->current_edge->top != (*bnd)->current_edge->top && (*bnd_next)->ring &&
        (*bnd_next)->winding_delta != 0 && (*bnd)->ring && (*bnd)->winding_delta != 0) {
        add_point_to_ring(bnd_next, (*bnd)->current_edge->top);
    }

    if (!(*bnd)->ring && !(*bndMaxPair)->ring) {
        active_bounds.erase(bndMaxPair);
        return active_bounds.erase(bnd);
    } else if ((*bnd)->ring && (*bndMaxPair)->ring) {
        add_local_maximum_point(bnd, bndMaxPair, (*bnd)->current_edge->top, rings, active_bounds);
        active_bounds.erase(bndMaxPair);
        return active_bounds.erase(bnd);
    } else if ((*bnd)->winding_delta == 0 && (*bnd)->ring) {
        add_point_to_ring(bnd, (*bnd)->current_edge->top);
        active_bounds.erase(bndMaxPair);
        return active_bounds.erase(bnd);
    } else if ((*bnd)->winding_delta == 0 && (*bndMaxPair)->ring) {
        add_point_to_ring(bndMaxPair, (*bnd)->current_edge->top);
        active_bounds.erase(bndMaxPair);
        return active_bounds.erase(bnd);
    } else {
        throw clipper_exception("DoMaxima error");
    }
}

template <typename T>
void process_edges_at_top_of_scanbeam(T top_y,
                                      active_bound_list<T>& active_bounds,
                                      scanbeam_list<T>& scanbeam,
                                      local_minimum_ptr_list<T> const& minima_sorted,
                                      local_minimum_ptr_list_itr<T>& current_lm,
                                      ring_list<T>& rings,
                                      join_list<T>& joins,
                                      clip_type cliptype,
                                      fill_type subject_fill_type,
                                      fill_type clip_fill_type) {
    if (active_bounds.empty()) {
        return;
    }

    maxima_list<T> maxima;
    for (auto bnd = active_bounds.begin(); bnd != active_bounds.end();) {
        // 1. Process maxima, treating them as if they are "bent" horizontal edges,
        // but exclude maxima with horizontal edges.

        bool is_maxima_edge = is_maxima(bnd, top_y);

        active_bound_list_itr<T> bnd_max_pair;
        if (is_maxima_edge) {
            bnd_max_pair = get_maxima_pair(bnd, active_bounds);
            is_maxima_edge =
                (bnd_max_pair == active_bounds.end() || !current_edge_is_horizontal<T>(bnd_max_pair));
        }

        if (is_maxima_edge) {
            maxima.push_back((*bnd)->current_edge->top.x);
            bnd = do_maxima(bnd, bnd_max_pair, cliptype, subject_fill_type, clip_fill_type, rings,
                            joins, active_bounds);
        } else {
            // 2. Promote horizontal edges. If not horizontal, update curr.x and curr.y.

            if (is_intermediate(bnd, top_y) && next_edge_is_horizontal<T>(bnd)) {
                next_edge_in_bound(bnd, scanbeam);
                if ((*bnd)->ring) {
                    add_point_to_ring(bnd, (*bnd)->current_edge->bot);
                    maxima.push_back((*bnd)->current_edge->top.x);
                    maxima.push_back((*bnd)->current_edge->bot.x);
                }
            } else {
                (*bnd)->curr.x = get_current_x(*((*bnd)->current_edge), top_y);
                (*bnd)->curr.y = top_y;
            }

            // When E is being touched by another edge, make sure both edges have a vertex here.
            if ((*bnd)->ring && (*bnd)->winding_delta != 0) {
                auto bnd_prev = active_bound_list_rev_itr<T>(bnd);
                while (bnd_prev != active_bounds.rend() && (*bnd_prev)->curr.x == (*bnd)->curr.x) {
                    if ((*bnd_prev)->ring && (*bnd_prev)->winding_delta != 0 &&
                        !((*bnd)->current_edge->bot == (*bnd_prev)->current_edge->bot && (*bnd)->current_edge->top == (*bnd_prev)->current_edge->top)) {
                        mapbox::geometry::point<T> pt = (*bnd)->curr;
                        point_ptr<T> op = add_point_to_ring(bnd_prev, pt);
                        point_ptr<T> op2 = add_point_to_ring(bnd, pt);
                        joins.emplace_back(op, op2, pt); // strictly simple type 3 join
                    }
                    ++bnd_prev;
                }
            }
            ++bnd;
        }
    }

    insert_horizontal_local_minima_into_ABL(top_y, minima_sorted, current_lm, active_bounds, rings,
                                            joins, scanbeam, cliptype, subject_fill_type,
                                            clip_fill_type, maxima);

    auto lm = current_lm;
    while (lm != minima_sorted.end() && (*lm)->y == top_y) {
        if (!(*lm)->left_bound.edges.empty() && !(*lm)->right_bound.edges.empty()) {
            maxima.push_back((*lm)->left_bound.edges.front().bot.x);
        }
        ++lm;
    }

    process_horizontals(top_y, maxima, active_bounds, joins, rings, scanbeam, cliptype, subject_fill_type,
                        clip_fill_type);

    // 4. Promote intermediate vertices

    for (auto bnd = active_bounds.begin(); bnd != active_bounds.end(); ++bnd) {
        if (is_intermediate(bnd, top_y)) {
            point_ptr<T> p1 = nullptr;
            if ((*bnd)->ring) {
                p1 = add_point_to_ring(bnd, (*bnd)->current_edge->top);
            }
            next_edge_in_bound(bnd, scanbeam);

            // If output polygons share an edge, they'll need to be joined later.

            if ((*bnd)->winding_delta != 0) {
                auto bnd_prev = active_bound_list_rev_itr<T>(bnd);
                auto bnd_next = std::next(bnd);

                if (bnd_prev != active_bounds.rend() && (*bnd_prev)->curr == (*bnd)->current_edge->bot && 
                    (*bnd_prev)->winding_delta != 0 &&
                    (*bnd_prev)->ring && (*bnd_prev)->curr.y > (*bnd_prev)->current_edge->top.y &&
                     slopes_equal(*((*bnd)->current_edge), *((*bnd_prev)->current_edge))) {
                    point_ptr<T> p2 = add_point_to_ring(bnd_prev, (*bnd)->current_edge->bot);
                    joins.emplace_back(p1, p2, (*bnd)->current_edge->top);
                } else if (bnd_next != active_bounds.end() && (*bnd_next)->curr == (*bnd)->current_edge->bot &&
                           (*bnd_next)->winding_delta != 0 &&
                           (*bnd_next)->ring && (*bnd_next)->curr.y > (*bnd_next)->current_edge->top.y &&
                           slopes_equal(*((*bnd)->current_edge), *((*bnd_next)->current_edge))) {
                    point_ptr<T> p2 = add_point_to_ring(bnd_next, (*bnd)->current_edge->bot);
                    joins.emplace_back(p1, p2, (*bnd)->current_edge->top);
                }
            }
        }
    }
}

template <typename T>
void fixup_out_polyline(ring<T>& ring) {
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
            delete pp;
            pp = tmpPP;
        }
    }

    if (pp == pp->prev) {
        dispose_out_points(pp);
        ring.points = nullptr;
        return;
    }
}

template <typename T>
void fixup_out_polygon(ring<T>& ring, ring_list<T>& rings, bool simple) {
    // FixupOutPolygon() - removes duplicate points and simplifies consecutive
    // parallel edges by removing the middle vertex.
    point_ptr<T> lastOK = nullptr;
    ring.bottom_point = nullptr;
    point_ptr<T> pp = ring.points;

    for (;;) {
        if (pp->prev == pp || pp->prev == pp->next) {
            // We now need to make sure any children rings to this are promoted and their hole
            // status is changed
            promote_children_of_removed_ring(&ring, rings);
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
            delete tmp;
        } else if (pp == lastOK)
            break;
        else {
            if (!lastOK) {
                lastOK = pp;
            }
            pp = pp->next;
        }
    }
    ring.points = pp;
}

template <typename T>
bool execute_vatti(local_minimum_list<T>& minima_list,
                   ring_list<T>& rings,
                   clip_type cliptype,
                   fill_type subject_fill_type,
                   fill_type clip_fill_type) {
    join_list<T> joins;

    if (minima_list.empty()) {
        return false;
    }

    {
        // This section in its own { } to limit memory scope of variables
        active_bound_list<T> active_bounds;
        scanbeam_list<T> scanbeam;
        T scanline_y;

        local_minimum_ptr_list<T> minima_sorted;
        minima_sorted.reserve(minima_list.size());
        for (auto& lm : minima_list) {
            minima_sorted.push_back(&lm);
        }
        std::stable_sort(minima_sorted.begin(), minima_sorted.end(), local_minimum_sorter<T>());
        local_minimum_ptr_list_itr<T> current_lm = minima_sorted.begin();

        setup_scanbeam(minima_list, scanbeam);

        while (pop_from_scanbeam(scanline_y, scanbeam) || current_lm != minima_sorted.end()) {

            // First we process bounds that has already been added to the active bound list --
            // if the active bound list is empty local minima that are at this scanline_y and
            // have a horizontal edge at the local minima will be processed
            process_edges_at_top_of_scanbeam(scanline_y, active_bounds, scanbeam,
                                             minima_sorted, current_lm, rings,
                                             joins, cliptype,
                                             subject_fill_type, clip_fill_type);

            // Next we will add local minima bounds to the active bounds list that are on the local
            // minima queue at
            // this current scanline_y
            insert_local_minima_into_ABL(scanline_y, minima_sorted, current_lm, active_bounds,
                                         rings, joins, scanbeam, cliptype, subject_fill_type,
                                         clip_fill_type);

            process_intersections(scanline_y, active_bounds, cliptype, subject_fill_type,
                                  clip_fill_type, rings, joins);
        }
    }

    for (auto& ring : rings) {
        if (!ring->points || ring->is_open) {
            continue;
        }

        if (ring->is_hole == (area(*ring) > 0)) {
            reverse_ring(ring->points);
        }
    }

    if (!joins.empty()) {
        join_common_edges(joins, rings);
    }

    // unfortunately FixupOutPolygon() must be done after JoinCommonEdges()
    for (size_t i = 0; i < rings.size(); ++i) {
        ring_ptr<T> ring = rings[i];
        if (!ring->points) {
            continue;
        }
        if (ring->is_open) {
            fixup_out_polyline(*ring);
        } else {
            fixup_out_polygon(*ring, rings, false);
        }
    }

    do_simple_polygons(rings);

    for (auto& ring : rings) {
        if (!ring->points || ring->is_open) {
            continue;
        }
        fixup_out_polygon(*ring, rings, true);
        if (ring->is_hole == (area(*ring) > 0)) {
            reverse_ring(ring->points);
        }
    }
    return true;
}
}
}
}
