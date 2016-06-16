#pragma once

#include <mapbox/geometry/wagyu/active_edge_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/intersect_util.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/local_minimum_util.hpp>
#include <mapbox/geometry/wagyu/process_horizontal.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void insert_local_minima_into_AEL(T const botY,
                                  local_minimum_itr<T>& current_local_min,
                                  local_minimum_list<T>& minima_list,
                                  edge_ptr<T>& active_edge_list,
                                  edge_ptr<T>& sorted_edge_list,
                                  ring_list<T>& rings,
                                  join_list<T>& joins,
                                  join_list<T>& ghost_joins,
                                  scanbeam_list<T>& scanbeam,
                                  clip_type cliptype,
                                  fill_type subject_fill_type,
                                  fill_type clip_fill_type) {
    using value_type = T;
    local_minimum_ptr<T> lm;
    while (pop_local_minima(botY, lm, current_local_min, minima_list)) {
        edge_ptr<value_type> lb = lm->left_bound;
        edge_ptr<value_type> rb = lm->right_bound;
        point_ptr<value_type> p1 = nullptr;
        if (!lb) {
            // nb: don't insert LB into either AEL or SEL
            insert_edge_into_AEL(rb, static_cast<edge_ptr<T>>(nullptr), active_edge_list);
            set_winding_count(*rb, cliptype, subject_fill_type, clip_fill_type, active_edge_list);
            if (is_contributing(*rb, cliptype, subject_fill_type, clip_fill_type)) {
                p1 = add_point(rb, rb->bot, rings);
                edge_ptr<value_type> eprev = rb->prev_in_AEL;
                if (rb->index >= 0 && rb->winding_delta != 0 && eprev && eprev->index >= 0 &&
                    eprev->curr.x == rb->curr.x && eprev->winding_delta != 0) {
                    add_point(eprev, rb->curr, rings);
                }
                edge_ptr<value_type> enext = rb->next_in_AEL;
                if (rb->index >= 0 && rb->winding_delta != 0 && enext && enext->index >= 0 &&
                    enext->curr.x == rb->curr.x && enext->winding_delta != 0) {
                    add_point(enext, rb->curr, rings);
                }
            }
        } else if (!rb) {
            insert_edge_into_AEL(lb, static_cast<edge_ptr<T>>(nullptr), active_edge_list);
            set_winding_count(*lb, cliptype, subject_fill_type, clip_fill_type, active_edge_list);
            if (is_contributing(*lb, cliptype, subject_fill_type, clip_fill_type)) {
                p1 = add_point(lb, lb->bot, rings);
                edge_ptr<value_type> eprev = lb->prev_in_AEL;
                if (lb->index >= 0 && lb->winding_delta != 0 && eprev && eprev->index >= 0 &&
                    eprev->curr.x == lb->curr.x && eprev->winding_delta != 0) {
                    add_point(eprev, lb->curr, rings);
                }
                edge_ptr<value_type> enext = lb->next_in_AEL;
                if (lb->index >= 0 && lb->winding_delta != 0 && enext && enext->index >= 0 &&
                    enext->curr.x == lb->curr.x && enext->winding_delta != 0) {
                    add_point(enext, lb->curr, rings);
                }
            }
            scanbeam.push(lb->top.y);
        } else {
            insert_edge_into_AEL(lb, static_cast<edge_ptr<T>>(nullptr), active_edge_list);
            insert_edge_into_AEL(rb, lb, active_edge_list);
            set_winding_count(*lb, cliptype, subject_fill_type, clip_fill_type, active_edge_list);
            rb->winding_count = lb->winding_count;
            rb->winding_count2 = lb->winding_count2;
            if (is_contributing(*lb, cliptype, subject_fill_type, clip_fill_type)) {
                p1 = add_local_minimum_point(lb, rb, lb->bot, rings, joins);
                edge_ptr<value_type> eprev = lb->prev_in_AEL;
                if (lb->index >= 0 && lb->winding_delta != 0 && eprev && eprev->index >= 0 &&
                    eprev->curr.x == lb->curr.x && eprev->winding_delta != 0) {
                    add_point(eprev, lb->curr, rings);
                }
                edge_ptr<value_type> enext = rb->next_in_AEL;
                if (rb->index >= 0 && rb->winding_delta != 0 && enext && enext->index >= 0 &&
                    enext->curr.x == rb->curr.x && enext->winding_delta != 0) {
                    add_point(enext, lb->curr, rings);
                }
            }
            scanbeam.push(lb->top.y);
        }

        if (rb) {
            if (is_horizontal(*rb)) {
                add_edge_to_SEL(rb, sorted_edge_list);
                if (rb->next_in_LML) {
                    scanbeam.push(rb->next_in_LML->top.y);
                }
            } else {
                scanbeam.push(rb->top.y);
            }
        }

        if (!lb || !rb) {
            continue;
        }

        // if any output polygons share an edge, they'll need joining later ...
        if (p1 && is_horizontal(*rb) && !ghost_joins.empty() && rb->winding_delta != 0) {
            for (auto jr = ghost_joins.begin(); jr != ghost_joins.end(); ++jr) {
                // if the horizontal Rb and a 'ghost' horizontal overlap, then
                // convert
                // the 'ghost' join to a real join ready for later ...
                if (horizontal_segments_overlap(jr->point1->x, jr->off_point.x, rb->bot.x,
                                                rb->top.x)) {
                    joins.emplace_back(jr->point1, p1, jr->off_point);
                }
            }
        }

        if (lb->index >= 0 && lb->prev_in_AEL && lb->prev_in_AEL->curr.x == lb->bot.x &&
            lb->prev_in_AEL->index >= 0 &&
            slopes_equal(lb->prev_in_AEL->bot, lb->prev_in_AEL->top, lb->curr, lb->top) &&
            lb->winding_delta != 0 && lb->prev_in_AEL->winding_delta != 0) {
            point_ptr<value_type> p2 = add_point(lb->prev_in_AEL, lb->bot, rings);
            joins.emplace_back(p1, p2, lb->top);
        }

        if (lb->next_in_AEL != rb) {
            if (rb->index >= 0 && rb->prev_in_AEL->index >= 0 &&
                slopes_equal(rb->prev_in_AEL->curr, rb->prev_in_AEL->top, rb->curr, rb->top) &&
                rb->winding_delta != 0 && rb->prev_in_AEL->winding_delta != 0) {
                point_ptr<value_type> p2 = add_point(rb->prev_in_AEL, rb->bot, rings);
                joins.emplace_back(p1, p2, rb->top);
            }

            edge_ptr<value_type> e = lb->next_in_AEL;
            if (e) {
                while (e != rb) {
                    // nb: For calculating winding counts etc, IntersectEdges()
                    // assumes
                    // that param1 will be to the Right of param2 ABOVE the
                    // intersection ...
                    // Note: order important here
                    intersect_edges(rb, e, lb->curr, cliptype, subject_fill_type, clip_fill_type,
                                    rings, joins, active_edge_list);
                    e = e->next_in_AEL;
                }
            }
        }
    }
}

template <typename T>
void do_maxima(edge_ptr<T> e,
               clip_type cliptype,
               fill_type subject_fill_type,
               fill_type clip_fill_type,
               ring_list<T>& rings,
               join_list<T>& joins,
               edge_ptr<T>& active_edge_list) {
    edge_ptr<T> eMaxPair = get_maxima_pair_ex(e);
    if (!eMaxPair) {
        if (e->index >= 0) {
            add_point(e, e->top, rings);
        }
        delete_from_AEL(e, active_edge_list);
        return;
    }

    edge_ptr<T> eprev = e->prev_in_AEL;
    if (eprev && eprev->curr.x == e->top.x && eprev->top != e->top && eprev->index >= 0 &&
        eprev->winding_delta != 0 && e->index >= 0 && e->winding_delta != 0) {
        add_point(eprev, e->top, rings);
    }
    edge_ptr<T> enext = e->next_in_AEL;
    while (enext && enext != eMaxPair) {
        intersect_edges(e, enext, e->top, cliptype, subject_fill_type, clip_fill_type, rings, joins,
                        active_edge_list);
        swap_positions_in_AEL(e, enext, active_edge_list);
        enext = e->next_in_AEL;
    }
    enext = eMaxPair->next_in_AEL;
    if (enext && enext->curr.x == e->top.x && enext->top != e->top && enext->index >= 0 &&
        enext->winding_delta != 0 && e->index >= 0 && e->winding_delta != 0) {
        add_point(enext, e->top, rings);
    }

    if (e->index == EDGE_UNASSIGNED && eMaxPair->index == EDGE_UNASSIGNED) {
        delete_from_AEL(e, active_edge_list);
        delete_from_AEL(eMaxPair, active_edge_list);
    } else if (e->index >= 0 && eMaxPair->index >= 0) {
        add_local_maximum_point(e, eMaxPair, e->top, rings, active_edge_list);
        delete_from_AEL(e, active_edge_list);
        delete_from_AEL(eMaxPair, active_edge_list);
    } else if (e->winding_delta == 0) {
        if (e->index >= 0) {
            add_point(e, e->top, rings);
            e->index = EDGE_UNASSIGNED;
        }
        delete_from_AEL(e, active_edge_list);

        if (eMaxPair->index >= 0) {
            add_point(eMaxPair, e->top, rings);
            eMaxPair->index = EDGE_UNASSIGNED;
        }
        delete_from_AEL(eMaxPair, active_edge_list);
    } else {
        throw clipper_exception("DoMaxima error");
    }
}

template <typename T>
void process_edges_at_top_of_scanbeam(T top_y,
                                      edge_ptr<T>& active_edge_list,
                                      edge_ptr<T>& sorted_edge_list,
                                      scanbeam_list<T>& scanbeam,
                                      maxima_list<T>& maxima,
                                      local_minimum_list<T>& minima_list,
                                      local_minimum_itr<T>& current_lm,
                                      ring_list<T>& rings,
                                      join_list<T>& joins,
                                      join_list<T>& ghost_joins,
                                      clip_type cliptype,
                                      fill_type subject_fill_type,
                                      fill_type clip_fill_type) {
    maxima_list<T> next_maxima;
    edge_ptr<T> e = active_edge_list;
    while (e) {
        // 1. Process maxima, treating them as if they are "bent" horizontal edges,
        // but exclude maxima with horizontal edges. E can't be a horizontal.

        bool is_maxima_edge = is_maxima(e, top_y);

        if (is_maxima_edge) {
            edge_ptr<T> e_max_pair = get_maxima_pair_ex(e);
            is_maxima_edge = (!e_max_pair || !is_horizontal(*e_max_pair));
        }

        if (is_maxima_edge) {
            maxima.push_back(e->top.x);
            next_maxima.push_back(e->top.x);
        }

        if (is_maxima_edge) {
            edge_ptr<T> e_prev = e->prev_in_AEL;
            do_maxima(e, cliptype, subject_fill_type, clip_fill_type, rings, joins,
                      active_edge_list);
            if (!e_prev) {
                e = active_edge_list;
            } else {
                e = e_prev->next_in_AEL;
            }
        } else {
            // 2. Promote horizontal edges. If not horizontal, update curr.x and curr.y.

            if (is_intermediate(e, top_y) && is_horizontal(*e->next_in_LML)) {
                update_edge_into_AEL(e, active_edge_list, scanbeam);
                if (e->index == 0) {
                    add_point(e, e->bot, rings);
                    maxima.push_back(e->top.x);
                    maxima.push_back(e->bot.x);
                    next_maxima.push_back(e->bot.x);
                }
                add_edge_to_SEL(e, sorted_edge_list);
            } else {
                e->curr.x = get_current_x(*e, top_y);
                e->curr.y = top_y;
            }
        }

        // When E is being touched by another edge, make sure both edges have a vertex here.

        if (e->index >= 0 && e->winding_delta != 0) {
            edge_ptr<T> e_prev = e->prev_in_AEL;
            while (e_prev && e->prev->curr.x == e->curr.x) {
                if (e_prev->index >= 0 && e_prev->winding_delta != 0 &&
                    !(e->bot == e_prev->bot && e->top == e_prev->top)) {
                    mapbox::geometry::point<T> pt = e->curr;
                    point_ptr<T> op = add_point(e_prev, pt, rings);
                    point_ptr<T> op2 = add_point(e, pt, rings);
                    joins.emplace_back(op, op2, pt); // strictly simple type 3 join
                }
                e_prev = e_prev->prev_in_AEL;
            }
        }

        e = e->next_in_AEL;
    }

    local_minimum_itr<T> lm = current_lm;
    while (lm != minima_list.end() && lm->y == top_y) {
        if (lm->left_bound && lm->right_bound) {
            maxima.push_back(lm->left_bound->bot.x);
        }
        ++lm;
    }

    process_horizontals(maxima, active_edge_list, sorted_edge_list, cliptype, subject_fill_type,
                        clip_fill_type, rings, joins, ghost_joins, scanbeam);

    if (!next_maxima.empty()) {
        maxima.insert(maxima.end(), next_maxima.begin(), next_maxima.end());
    }

    // 4. Promote intermediate vertices

    e = active_edge_list;
    while (e) {
        if (is_intermediate(e, top_y)) {
            point_ptr<T> op = 0;
            if (e->index >= 0) {
                op = add_point(e, e->top, rings);
            }
            update_edge_into_AEL(e, active_edge_list, scanbeam);

            // If output polygons share an edge, they'll need to be joined later.

            edge_ptr<T> e_prev = e->prev_in_AEL;
            edge_ptr<T> e_next = e->next_in_AEL;

            if (e_prev && e_prev->curr.x == e->bot.x && e_prev->index >= 0 &&
                e_prev->curr.y > e_prev->top.y &&
                slopes_equal(e->curr, e->top, e_prev->curr, e_prev->top) &&
                (e->winding_delta != 0) && (e_prev->winding_delta != 0)) {
                point_ptr<T> op2 = add_point(e_prev, e->bot, rings);
                joins.emplace_back(op, op2, e->top);
            } else if (e_next && e_next->curr.x == e->bot.x && e_next->curr.y == e->bot.y &&
                       e->next->index >= 0 && e_next->curr.y > e_next->top.y &&
                       slopes_equal(e->curr, e->top, e_next->curr, e_next->top) &&
                       (e->winding_delta != 0) && (e_next->winding_delta != 0)) {
                point_ptr<T> op2 = add_point(e_next, e->bot, rings);
                joins.emplace_back(op, op2, e->top);
            }
        }
        e = e->next_in_AEL;
    }
}

template <typename T>
void setup_scanbeam(local_minimum_list<T>& minima_list, scanbeam_list<T>& scanbeam) {
    if (minima_list.empty()) {
        return; // ie nothing to process
    }
    std::stable_sort(minima_list.begin(), minima_list.end(), local_minimum_sorter<T>());

    // reset all edges ...
    for (auto lm = minima_list.begin(); lm != minima_list.end(); ++lm) {
        scanbeam.push(lm->y);
        edge_ptr<T> e = lm->left_bound;
        if (e) {
            e->curr = e->bot;
            e->side = edge_left;
            e->index = EDGE_UNASSIGNED;
        }

        e = lm->right_bound;
        if (e) {
            e->curr = e->bot;
            e->side = edge_right;
            e->index = EDGE_UNASSIGNED;
        }
    }
}

template <typename T>
bool pop_from_scanbeam(T& Y, scanbeam_list<T>& scanbeam) {
    if (scanbeam.empty()) {
        return false;
    }
    Y = scanbeam.top();
    scanbeam.pop();
    while (!scanbeam.empty() && Y == scanbeam.top()) {
        scanbeam.pop();
    } // Pop duplicates.
    return true;
}

template <typename T>
void process_horizontals(maxima_list<T>& maxima,
                         edge_ptr<T>& active_edge_list,
                         edge_ptr<T>& sorted_edge_list,
                         clip_type cliptype,
                         fill_type subject_fill_type,
                         fill_type clip_fill_type,
                         ring_list<T> rings,
                         join_list<T>& joins,
                         join_list<T>& ghost_joins,
                         scanbeam_list<T>& scanbeam) {
    maxima.sort();
    edge_ptr<T> horz_edge;
    while (pop_edge_from_SEL(horz_edge, sorted_edge_list)) {
        process_horizontal(horz_edge, maxima, active_edge_list, sorted_edge_list, cliptype,
                           subject_fill_type, clip_fill_type, rings, joins, ghost_joins, scanbeam);
    }
    maxima.clear();
}

template <typename T>
ring_ptr<T> get_ring(ring_list<T>& rings, int index) {
    ring_ptr<T> r = rings[index];
    while (r != rings[r->index]) {
        r = rings[r->index];
    }
    return r;
}

template <typename T>
ring_ptr<T> create_ring(ring_list<T>& rings) {
    ring_ptr<T> r = new ring<T>;
    r->is_hole = false;
    r->is_open = false;
    r->first_left = 0;
    r->points = 0;
    r->bottom_point = 0;
    r->poly_node = 0;
    rings.push_back(r);
    r->index = rings.size() - 1;
    return r;
}

template <typename T>
int point_count(point_ptr<T> points) {
    if (!points) {
        return 0;
    }

    int n = 0;
    point_ptr<T> p = points;
    do {
        n++;
        p = p->next;
    } while (p != points);

    return n;
}

template <typename T>
void join_common_edges(join_list<T>& joins, ring_list<T>& rings) {
    for (size_t i = 0; i < joins.size(); i++) {
        join_ptr<T> join = &joins[i];

        ring_ptr<T> ring1 = get_ring(rings, join->point1->index);
        ring_ptr<T> ring2 = get_ring(rings, join->point2->index);

        if (!ring1->points || !ring2->points) {
            continue;
        }
        if (ring1->is_open || ring2->is_open) {
            continue;
        }

        // Get the polygon fragment with the corringt hole state (FirstLeft)
        // before calling join_points().

        ring_ptr<T> hole_state_ring;
        if (ring1 == ring2) {
            hole_state_ring = ring1;
        } else if (ring1_right_of_ring2(ring1, ring2)) {
            hole_state_ring = ring2;
        } else if (ring1_right_of_ring2(ring2, ring1)) {
            hole_state_ring = ring1;
        } else {
            hole_state_ring = get_lowermost_ring(ring1, ring2);
        }

// XXX ENF left off here
#if 0
        if (!join_points(join, ring1, ring2)) {
            continue;
        }
#endif

        if (ring1 == ring2) {
            // Instead of joining two polygons, we have created a new one
            // by splitting one polygon into two.

            ring1->bottom_point = 0;
            ring2 = create_ring(rings);

            if (point_count(join->point1) > point_count(join->point2)) {
                ring1->points = join->point1;
                ring2->points = join->point2;
            } else {
                ring1->points = join->point2;
                ring2->points = join->point1;
            }

// Update indices in points in ring2
// XXX ENF left off here
#if 0
            update_out_point_indices(*ring2);

            if (poly2_contains_poly1(ring2->points, ring1->points)) {
                // ring 1 contains ring 2

                ring2->is_hole = !ring1->is_hole;
                ring2->first_left = ring1;

                // XXX ENF left off here
            }
#endif
        }
    }
}

template <typename T>
bool execute_vatti(local_minimum_list<T>& minima_list,
                   ring_list<T> rings,
                   clip_type cliptype,
                   fill_type subject_fill_type,
                   fill_type clip_fill_type) {
    using value_type = T;
    edge_ptr<value_type> active_edge_list = nullptr;
    edge_ptr<value_type> sorted_edge_list = nullptr;
    scanbeam_list<value_type> scanbeam;
    maxima_list<value_type> max_list;
    value_type bot_y;
    value_type top_y;
    join_list<value_type> joins;
    join_list<value_type> ghost_joins;

    setup_scanbeam(minima_list, scanbeam);

    local_minimum_itr<T> current_local_min = minima_list.begin();

    if (!pop_from_scanbeam(bot_y, scanbeam)) {
        return false;
    }
    insert_local_minima_into_AEL(bot_y, current_local_min, minima_list, active_edge_list,
                                 sorted_edge_list, rings, joins, ghost_joins, scanbeam, cliptype,
                                 subject_fill_type, clip_fill_type);
    while (pop_from_scanbeam(top_y, scanbeam) ||
           local_minima_pending(current_local_min, minima_list)) {
        process_horizontals(max_list, active_edge_list, sorted_edge_list, cliptype,
                            subject_fill_type, clip_fill_type, rings, joins, ghost_joins, scanbeam);
        ghost_joins.clear();

        if (!process_intersections(top_y, active_edge_list, sorted_edge_list, cliptype,
                                   subject_fill_type, clip_fill_type, rings, joins)) {
            return false;
        }
        process_edges_at_top_of_scanbeam(top_y, active_edge_list, sorted_edge_list, scanbeam,
                                         max_list, minima_list, current_local_min, rings, joins,
                                         ghost_joins, cliptype, subject_fill_type, clip_fill_type);
        bot_y = top_y;
        insert_local_minima_into_AEL(bot_y, current_local_min, minima_list, active_edge_list,
                                     sorted_edge_list, rings, joins, ghost_joins, scanbeam,
                                     cliptype, subject_fill_type, clip_fill_type);
    }

    for (size_t i = 0; i < rings.size(); ++i) {
        ring_ptr<T> outrec = rings[i];

        if (!outrec->points || outrec->is_open) {
            continue;
        }

        // left out m_ReverseOutput
        if (outrec->is_hole == (area(*outrec) > 0)) {
            reverse_ring(outrec->points);
        }
    }

    if (!joins.empty()) {
        join_common_edges(joins, rings);
    }

    return true;
}
}
}
}
