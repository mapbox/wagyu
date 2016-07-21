#pragma once

#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
bool horizontal_segments_overlap(T seg1a, T seg1b, T seg2a, T seg2b) {
    if (seg1a > seg1b) {
        if (seg2a > seg2b) {
            return (seg1b < seg2a) && (seg2b < seg1a);
        } else {
            return (seg1b < seg2b) && (seg2a < seg1a);
        }
    } else {
        if (seg2a > seg2b) {
            return (seg1a < seg2a) && (seg2b < seg1b);
        } else {
            return (seg1a < seg2b) && (seg2a < seg1b);
        }
    }
}

template <typename T>
point_ptr<T> get_last_point(active_bound_list_itr<T>& bnd) {
    ring_ptr<T> outRec = (*bnd)->ring;
    if ((*bnd)->side == edge_left) {
        return outRec->points;
    } else {
        return outRec->points->prev;
    }
}

template <typename T>
active_bound_list_itr<T> process_horizontal_left_to_right(T scanline_y,
                                                          active_bound_list_itr<T> horz_bound,
                                                          maxima_list<T>& maxima,
                                                          active_bound_list<T>& active_bounds,
                                                          join_list<T>& joins,
                                                          ring_list<T>& rings,
                                                          scanbeam_list<T>& scanbeam,
                                                          clip_type cliptype,
                                                          fill_type subject_fill_type,
                                                          fill_type clip_fill_type) {
    auto horizontal_itr_behind = horz_bound;
    bool is_open = (*horz_bound)->winding_delta == 0;
    bool is_maxima_edge = is_maxima(horz_bound, scanline_y);
    auto bound_max_pair = active_bounds.end();
    if (is_maxima_edge) {
        bound_max_pair = get_maxima_pair<T>(horz_bound, active_bounds);
    }

    auto max_iter = maxima.begin();
    while (max_iter != maxima.end() && *max_iter <= (*horz_bound)->current_edge->bot.x) {
        ++max_iter;
    }

    point_ptr<T> p1 = nullptr;

    auto bnd = std::next(horz_bound);

    while (bnd != active_bounds.end()) {
        // this code block inserts extra coords into horizontal edges (in output
        // polygons) wherever maxima touch these horizontal edges. This helps
        //'simplifying' polygons (ie if the Simplify property is set).
        while (max_iter != maxima.end() && *max_iter < (*bnd)->curr.x) {
            if ((*horz_bound)->ring && !is_open) {
                add_point_to_ring(horz_bound, mapbox::geometry::point<T>(
                                                  *max_iter, (*horz_bound)->current_edge->bot.y));
            }
            ++max_iter;
        }

        if ((*bnd)->curr.x > (*horz_bound)->current_edge->top.x) {
            break;
        }

        // Also break if we've got to the end of an intermediate horizontal edge ...
        // nb: Smaller Dx's are to the right of larger Dx's ABOVE the horizontal.
        if ((*bnd)->curr.x == (*horz_bound)->current_edge->top.x &&
            std::next((*horz_bound)->current_edge) != (*horz_bound)->edges.end() &&
            (*horz_bound)->current_edge->dx < std::next((*horz_bound)->current_edge)->dx) {
            break;
        }

        // note: may be done multiple times
        if ((*horz_bound)->ring && !is_open) {
            p1 = add_point_to_ring(horz_bound, (*bnd)->curr);
            for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
                if (bnd_itr != horz_bound && (*bnd_itr)->ring &&
                    current_edge_is_horizontal<T>(bnd_itr) &&
                    horizontal_segments_overlap(
                        (*horz_bound)->current_edge->bot.x, (*horz_bound)->current_edge->top.x,
                        (*bnd_itr)->current_edge->bot.x, (*bnd_itr)->current_edge->top.x)) {
                    point_ptr<T> p2 = get_last_point<T>(bnd_itr);
                    joins.emplace_back(p2, p1, (*bnd_itr)->current_edge->top);
                }
            }
        }

        // OK, so far we're still in range of the horizontal Edge  but make sure
        // we're at the last of consec. horizontals when matching with eMaxPair
        if (is_maxima_edge && bnd == bound_max_pair) {
            if ((*horz_bound)->ring) {
                add_local_maximum_point(horz_bound, bound_max_pair,
                                        (*horz_bound)->current_edge->top, rings, active_bounds);
            }
            active_bounds.erase(bound_max_pair);
            auto after_horz = active_bounds.erase(horz_bound);
            if (horizontal_itr_behind != horz_bound) {
                return horizontal_itr_behind;
            } else {
                return after_horz;
            }
        }

        intersect_bounds(horz_bound, bnd,
                         mapbox::geometry::point<T>((*bnd)->curr.x, (*horz_bound)->curr.y),
                         cliptype, subject_fill_type, clip_fill_type, rings, joins, active_bounds);
        auto next_bnd = std::next(bnd);
        swap_positions_in_ABL(horz_bound, bnd, active_bounds);
        if (current_edge_is_horizontal<T>(bnd) && horizontal_itr_behind == horz_bound) {
            horizontal_itr_behind = bnd;
        }
        bnd = next_bnd;
    } // end while (bnd != active_bounds.end())

    if ((*horz_bound)->ring && !is_open) {
        while (max_iter != maxima.end() && *max_iter < (*horz_bound)->current_edge->top.x) {
            add_point_to_ring(horz_bound, mapbox::geometry::point<T>(
                                              *max_iter, (*horz_bound)->current_edge->bot.y));
            ++max_iter;
        }
    }

    if ((*horz_bound)->ring && !p1) {
        p1 = get_last_point<T>(horz_bound);
        for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
            if (bnd_itr != horz_bound && (*bnd_itr)->ring &&
                current_edge_is_horizontal<T>(bnd_itr) &&
                horizontal_segments_overlap(
                    (*horz_bound)->current_edge->bot.x, (*horz_bound)->current_edge->top.x,
                    (*bnd_itr)->current_edge->bot.x, (*bnd_itr)->current_edge->top.x)) {
                point_ptr<T> p2 = get_last_point<T>(bnd_itr);
                joins.emplace_back(p2, p1, (*bnd_itr)->current_edge->top);
            }
        }
    }

    if (std::next((*horz_bound)->current_edge) != (*horz_bound)->edges.end()) {
        if ((*horz_bound)->ring) {
            p1 = add_point_to_ring(horz_bound, (*horz_bound)->current_edge->top);
            if ((*horz_bound)->winding_delta != 0) {
                auto bnd_prev = active_bound_list_rev_itr<T>(horz_bound);
                auto bnd_next = std::next(horz_bound);
                if (bnd_prev != active_bounds.rend() && (*bnd_prev)->ring &&
                    (*bnd_prev)->curr.x == (*horz_bound)->current_edge->top.x &&
                    (*bnd_prev)->winding_delta != 0) {
                    add_point_to_ring(bnd_prev, (*horz_bound)->current_edge->top);
                }
                if (bnd_next != active_bounds.end() && (*bnd_next)->ring &&
                    (*bnd_next)->curr.x == (*horz_bound)->current_edge->top.x &&
                    (*bnd_next)->winding_delta != 0) {
                    add_point_to_ring(bnd_next, (*horz_bound)->current_edge->top);
                }
            }
            next_edge_in_bound(horz_bound, scanbeam);

            if ((*horz_bound)->winding_delta == 0) {
                if (horizontal_itr_behind != horz_bound) {
                    return horizontal_itr_behind;
                } else {
                    return std::next(horz_bound);
                }
            }

            // horz_bound is no longer horizontal here!!! the edge
            // in the bound was updated above
            auto bnd_prev = active_bound_list_rev_itr<T>(horz_bound);
            auto bnd_next = std::next(horz_bound);

            if (bnd_prev != active_bounds.rend() &&
                (*bnd_prev)->curr == (*horz_bound)->current_edge->bot &&
                (*bnd_prev)->winding_delta != 0 && (*bnd_prev)->ring &&
                (*bnd_prev)->curr.y > (*bnd_prev)->current_edge->top.y &&
                slopes_equal(*((*horz_bound)->current_edge), *((*bnd_prev)->current_edge))) {
                point_ptr<T> p2 = add_point_to_ring(bnd_prev, (*horz_bound)->current_edge->bot);
                joins.emplace_back(p1, p2, (*horz_bound)->current_edge->top);
            } else if (bnd_next != active_bounds.end() &&
                       (*bnd_next)->curr == (*horz_bound)->current_edge->bot &&
                       (*bnd_next)->winding_delta != 0 && (*bnd_next)->ring &&
                       (*bnd_next)->curr.y > (*bnd_next)->current_edge->top.y &&
                       slopes_equal(*((*horz_bound)->current_edge), *((*bnd_next)->current_edge))) {
                point_ptr<T> p2 = add_point_to_ring(bnd_next, (*horz_bound)->current_edge->bot);
                joins.emplace_back(p1, p2, (*horz_bound)->current_edge->top);
            }
        } else {
            next_edge_in_bound(horz_bound, scanbeam);
        }
        if (horizontal_itr_behind != horz_bound) {
            return horizontal_itr_behind;
        } else {
            return std::next(horz_bound);
        }
    } else {
        if ((*horz_bound)->ring) {
            add_point_to_ring(horz_bound, (*horz_bound)->current_edge->top);
        }
        auto after_horz = active_bounds.erase(horz_bound);
        if (horizontal_itr_behind != horz_bound) {
            return horizontal_itr_behind;
        } else {
            return after_horz;
        }
    }
}

template <typename T>
active_bound_list_itr<T> process_horizontal_right_to_left(T scanline_y,
                                                          active_bound_list_itr<T> horz_bound,
                                                          maxima_list<T>& maxima,
                                                          active_bound_list<T>& active_bounds,
                                                          join_list<T>& joins,
                                                          ring_list<T>& rings,
                                                          scanbeam_list<T>& scanbeam,
                                                          clip_type cliptype,
                                                          fill_type subject_fill_type,
                                                          fill_type clip_fill_type) {
    bool is_open = (*horz_bound)->winding_delta == 0;
    bool is_maxima_edge = is_maxima(horz_bound, scanline_y);
    auto bound_max_pair = active_bounds.end();
    if (is_maxima_edge) {
        bound_max_pair = get_maxima_pair<T>(horz_bound, active_bounds);
    }

    auto max_iter = maxima.rbegin();
    while (max_iter != maxima.rend() && *max_iter > (*horz_bound)->current_edge->bot.x) {
        ++max_iter;
    }

    point_ptr<T> p1 = nullptr;

    auto bnd = active_bound_list_rev_itr<T>(horz_bound);

    while (bnd != active_bounds.rend()) {
        // this code block inserts extra coords into horizontal edges (in output
        // polygons) wherever maxima touch these horizontal edges. This helps
        //'simplifying' polygons (ie if the Simplify property is set).
        while (max_iter != maxima.rend() && *max_iter > (*bnd)->curr.x) {
            if ((*horz_bound)->ring && !is_open) {
                add_point_to_ring(horz_bound, mapbox::geometry::point<T>(
                                                  *max_iter, (*horz_bound)->current_edge->bot.y));
            }
            ++max_iter;
        }

        if ((*bnd)->curr.x < (*horz_bound)->current_edge->top.x) {
            break;
        }

        // Also break if we've got to the end of an intermediate horizontal edge ...
        // nb: Smaller Dx's are to the right of larger Dx's ABOVE the horizontal.
        if ((*bnd)->curr.x == (*horz_bound)->current_edge->top.x &&
            std::next((*horz_bound)->current_edge) != (*horz_bound)->edges.end() &&
            (*horz_bound)->current_edge->dx < std::next((*horz_bound)->current_edge)->dx) {
            break;
        }

        // note: may be done multiple times
        if ((*horz_bound)->ring && !is_open) {
            p1 = add_point_to_ring(horz_bound, (*bnd)->curr);
            for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
                if (bnd_itr != horz_bound && (*bnd_itr)->ring &&
                    current_edge_is_horizontal<T>(bnd_itr) &&
                    horizontal_segments_overlap(
                        (*horz_bound)->current_edge->bot.x, (*horz_bound)->current_edge->top.x,
                        (*bnd_itr)->current_edge->bot.x, (*bnd_itr)->current_edge->top.x)) {
                    point_ptr<T> p2 = get_last_point<T>(bnd_itr);
                    joins.emplace_back(p2, p1, (*bnd_itr)->current_edge->top);
                }
            }
        }
        auto bnd_forward = --(bnd.base());

        // OK, so far we're still in range of the horizontal Edge  but make sure
        // we're at the last of consec. horizontals when matching with eMaxPair
        if (is_maxima_edge && bnd_forward == bound_max_pair) {
            if ((*horz_bound)->ring) {
                add_local_maximum_point(horz_bound, bound_max_pair,
                                        (*horz_bound)->current_edge->top, rings, active_bounds);
            }
            active_bounds.erase(bound_max_pair);
            return active_bounds.erase(horz_bound);
        }

        intersect_bounds(bnd_forward, horz_bound,
                         mapbox::geometry::point<T>((*bnd)->curr.x, (*horz_bound)->curr.y),
                         cliptype, subject_fill_type, clip_fill_type, rings, joins, active_bounds);
        auto next_bnd = std::next(bnd);
        swap_positions_in_ABL(horz_bound, bnd_forward, active_bounds);
        bnd = next_bnd;
    } // end while (bnd != active_bounds.rend())

    if ((*horz_bound)->ring && !is_open) {
        while (max_iter != maxima.rend() && *max_iter > (*horz_bound)->current_edge->top.x) {
            add_point_to_ring(horz_bound, mapbox::geometry::point<T>(
                                              *max_iter, (*horz_bound)->current_edge->bot.y));
            ++max_iter;
        }
    }

    if ((*horz_bound)->ring && !p1) {
        p1 = get_last_point<T>(horz_bound);
        for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
            if (bnd_itr != horz_bound && (*bnd_itr)->ring &&
                current_edge_is_horizontal<T>(bnd_itr) &&
                horizontal_segments_overlap(
                    (*horz_bound)->current_edge->bot.x, (*horz_bound)->current_edge->top.x,
                    (*bnd_itr)->current_edge->bot.x, (*bnd_itr)->current_edge->top.x)) {
                point_ptr<T> p2 = get_last_point<T>(bnd_itr);
                joins.emplace_back(p2, p1, (*bnd_itr)->current_edge->top);
            }
        }
    }

    if (std::next((*horz_bound)->current_edge) != (*horz_bound)->edges.end()) {
        if ((*horz_bound)->ring) {
            p1 = add_point_to_ring(horz_bound, (*horz_bound)->current_edge->top);
            if ((*horz_bound)->winding_delta != 0) {
                auto bnd_prev = active_bound_list_rev_itr<T>(horz_bound);
                auto bnd_next = std::next(horz_bound);
                if (bnd_prev != active_bounds.rend() && (*bnd_prev)->ring &&
                    (*bnd_prev)->curr.x == (*horz_bound)->current_edge->top.x &&
                    (*bnd_prev)->winding_delta != 0) {
                    add_point_to_ring(bnd_prev, (*horz_bound)->current_edge->top);
                }
                if (bnd_next != active_bounds.end() && (*bnd_next)->ring &&
                    (*bnd_next)->curr.x == (*horz_bound)->current_edge->top.x &&
                    (*bnd_next)->winding_delta != 0) {
                    add_point_to_ring(bnd_next, (*horz_bound)->current_edge->top);
                }
            }
            next_edge_in_bound(horz_bound, scanbeam);

            if ((*horz_bound)->winding_delta == 0) {
                return std::next(horz_bound);
            }

            // horz_bound is no longer horizontal here!!! the edge
            // in the bound was updated above
            auto bnd_prev = active_bound_list_rev_itr<T>(horz_bound);
            auto bnd_next = std::next(horz_bound);

            if (bnd_prev != active_bounds.rend() &&
                (*bnd_prev)->curr == (*horz_bound)->current_edge->bot &&
                (*bnd_prev)->winding_delta != 0 && (*bnd_prev)->ring &&
                (*bnd_prev)->curr.y > (*bnd_prev)->current_edge->top.y &&
                slopes_equal(*((*horz_bound)->current_edge), *((*bnd_prev)->current_edge))) {
                point_ptr<T> p2 = add_point_to_ring(bnd_prev, (*horz_bound)->current_edge->bot);
                joins.emplace_back(p1, p2, (*horz_bound)->current_edge->top);
            } else if (bnd_next != active_bounds.end() &&
                       (*bnd_next)->curr == (*horz_bound)->current_edge->bot &&
                       (*bnd_next)->winding_delta != 0 && (*bnd_next)->ring &&
                       (*bnd_next)->curr.y > (*bnd_next)->current_edge->top.y &&
                       slopes_equal(*((*horz_bound)->current_edge), *((*bnd_next)->current_edge))) {
                point_ptr<T> p2 = add_point_to_ring(bnd_next, (*horz_bound)->current_edge->bot);
                joins.emplace_back(p1, p2, (*horz_bound)->current_edge->top);
            }
        } else {
            next_edge_in_bound(horz_bound, scanbeam);
        }
        return std::next(horz_bound);
    } else {
        if ((*horz_bound)->ring) {
            add_point_to_ring(horz_bound, (*horz_bound)->current_edge->top);
        }
        return active_bounds.erase(horz_bound);
    }
}

template <typename T>
active_bound_list_itr<T> process_horizontal(T scanline_y,
                                            active_bound_list_itr<T> horz_bound,
                                            maxima_list<T>& maxima,
                                            active_bound_list<T>& active_bounds,
                                            join_list<T>& joins,
                                            ring_list<T>& rings,
                                            scanbeam_list<T>& scanbeam,
                                            clip_type cliptype,
                                            fill_type subject_fill_type,
                                            fill_type clip_fill_type) {
    if ((*horz_bound)->current_edge->bot.x < (*horz_bound)->current_edge->top.x) {
        return process_horizontal_left_to_right(scanline_y, horz_bound, maxima, active_bounds,
                                                joins, rings, scanbeam, cliptype, subject_fill_type,
                                                clip_fill_type);
    } else {
        return process_horizontal_right_to_left(scanline_y, horz_bound, maxima, active_bounds,
                                                joins, rings, scanbeam, cliptype, subject_fill_type,
                                                clip_fill_type);
    }
}

template <typename T>
void process_horizontals(T scanline_y,
                         maxima_list<T>& maxima,
                         active_bound_list<T>& active_bounds,
                         join_list<T>& joins,
                         ring_list<T>& rings,
                         scanbeam_list<T>& scanbeam,
                         clip_type cliptype,
                         fill_type subject_fill_type,
                         fill_type clip_fill_type) {
    maxima.sort();
    for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end();) {
        if (current_edge_is_horizontal<T>(bnd_itr)) {
            bnd_itr = process_horizontal(scanline_y, bnd_itr, maxima, active_bounds, joins, rings,
                                         scanbeam, cliptype, subject_fill_type, clip_fill_type);
        } else {
            ++bnd_itr;
        }
    }
    maxima.clear();
}
}
}
}
