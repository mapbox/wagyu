#pragma once

#include <mapbox/geometry/wagyu/active_edge_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
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
            insert_edge_into_AEL(rb, nullptr, active_edge_list);
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
            insert_edge_into_AEL(lb, nullptr, active_edge_list);
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
            scanbeam.push_back(lb->top.y);
        } else {
            insert_edge_into_AEL(lb, nullptr, active_edge_list);
            insert_edge_into_AEL(rb, lb, active_edge_list);
            set_winding_count(*lb, cliptype, subject_fill_type, clip_fill_type, active_edge_list);
            rb->winding_count = lb->winding_count;
            rb->winding_count2 = lb->winding_count2;
            if (is_contributing(*lb, cliptype, subject_fill_type, clip_fill_type)) {
                p1 = add_local_minimum_point(lb, rb, lb->bot);
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
            scanbeam.push_back(lb->top.y);
        }

        if (rb) {
            if (is_horizontal(*rb)) {
                add_edge_to_SEL(rb, sorted_edge_list);
                if (rb->next_in_LML) {
                    scanbeam.push_back(rb->next_in_LML->top.y);
                }
            } else {
                scanbeam.push_back(rb->top.y);
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
void process_edges_at_top_of_scanbeam(T top_y,
                                      edge_ptr<T> active_edges,
                                      scanbeam_list<T>& scanbeam,
                                      std::list<T>& maxima,
                                      local_minimum_list<T>& minima_list,
                                      local_minimum_itr<T>& current_lm,
                                      ring_list<T>& rings,
                                      join_list<T>& joins) {
    std::list<T> next_maxima;
    edge_ptr<T> e = active_edges;
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
            do_maxima(e);
            if (!e_prev) {
                e = active_edges;
            } else {
                e = e_prev->next_in_AEL;
            }
        } else {
            // 2. Promote horizontal edges. If not horizontal, update curr.x and curr.y.

            if (is_intermediate(e, top_y) && is_horizontal(*e->next_in_LML)) {
                update_edge_into_AEL(e, active_edges, scanbeam);
                if (e->index == 0) {
                    add_point(e, e->bot, rings);
                    maxima.push_back(e->top.x);
                    maxima.push_back(e->bot.x);
                    next_maxima.push_back(e->bot.x);
                }
                add_edge_to_SEL(e);
            } else {
                e->curr.x = top_x(*e, top_y);
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

    process_horizontals(maxima);

    if (!next_maxima.empty()) {
        maxima.insert(maxima.end(), next_maxima.begin(), next_maxima.end());
    }

    // 4. Promote intermediate vertices

    e = active_edges;
    while (e) {
        if (is_intermediate(e, top_y)) {
            point_ptr<T> op = 0;
            if (e->index >= 0) {
                op = add_point(e, e->top, rings);
            }
            update_edge_into_AEL(e, active_edges, scanbeam);

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

bool execute_vatti() {
}
}
}
}
