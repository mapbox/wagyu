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
point_ptr<T> get_last_point(edge_ptr<T> e, ring_list<T> const& rings) {
    ring_ptr<T> outRec = rings[e->OutIdx];
    if (e->side == edge_left)
    {
        return outRec->points;
    }
    else
    {
        return outRec->points->prev;
    }
}

template <typename T>
void get_horizontal_direction(edge_ptr<T> edge, horizontal_direction& dir, T& left, T& right) {
    if (edge->bot.x < edge->top.x) {
        left = edge->bot.x;
        right = edge->top.x;
        dir = left_to_right; 
    } else {
        left = edge->top.x;
        right = edge->bot.x;
        dir = right_to_left;
    }
}

template <typename T>
void process_horizontal(edge_ptr<T> edge, 
    maxima_list<T>& maxima, 
    edge_ptr<T>& sorted_edge_list,
    edge_ptr<T>& active_edge_list,
    join_list<T>& joins,
    join_list<T>& ghost_joins,
    ring_list<T> rings,
    scanbeam_list<T>& scanbeam) 
{
    T left;
    T right;
    horizontal_direction dir;
    bool is_open = edge->winding_delta == 0;

    get_horizontal_direction(edge, dir, left, right);
    edge_ptr<T> last_horizontal = edge;
    edge_ptr<T> edge_max_pair = nullptr;

    // locate the final consecutive horizontal edge starting from this one;
    // multiple horizontals in a row will be linked
    while (last_horizontal->next_in_LML && is_horizontal(last_horizontal->next_in_LML)) {
        last_horizontal = last_horizontal->next_in_LML;
    }

    if (!last_horizontal->next_in_LML)
        edge_max_pair = get_maxima_pair(last_horizontal);

    typename maxima_list<T>::const_iterator max_iter;
    typename maxima_list<T>::const_reverse_iterator max_reverse_iter;

    if (!maxima.empty()) {
        //get the first maxima in range (X) ...
        if (dir == left_to_right) {
            max_iter = maxima.begin();
            while (max_iter != maxima.end() && *max_iter <= edge->bot.x) ++max_iter;
            if (max_iter != maxima.end() && *max_iter >= last_horizontal->top.x)
                max_iter = maxima.end();
        } else {
            max_reverse_iter = maxima.rbegin();
            while (max_reverse_iter != maxima.rend() && *max_reverse_iter > edge->bot.x) ++max_reverse_iter;
            if (max_reverse_iter != maxima.rend() && *max_reverse_iter <= last_horizontal->top.x)
                max_reverse_iter = maxima.rend();
        }
    }

    point_ptr<T> p1 = nullptr;

    // loop through consec. horizontal edges
    for (;;) {
        bool is_last_horizontal = (edge == last_horizontal);
        edge_ptr<T> e = get_next_in_AEL(edge, dir);

        while (e) {
            // this code block inserts extra coords into horizontal edges (in output
            // polygons) wherever maxima touch these horizontal edges. This helps
            //'simplifying' polygons (ie if the Simplify property is set).
            if (!maxima.empty()) {
                if (dir == left_to_right) {
                    while (max_iter != maxima.end() && *max_iter < e->curr.x) {
                        if (edge->index >= 0 && !is_open) {
                            add_point(edge, mapbox::geometry::point<T>(*max_iter, edge->bot.y), rings);
                        }
                        ++max_iter;
                    }
                } else {
                    while (max_reverse_iter != maxima.rend() && *max_reverse_iter > e->curr.x) {
                        if (edge->index >= 0 && !is_open)
                            add_point(edge, mapbox::geometry::point<T>(*max_reverse_iter, edge->bot.y));
                        ++max_reverse_iter;
                    }
                }
            };

            if ((dir == left_to_right && e->curr.x > right) ||
                (dir == right_to_left && e->curr.x < left)) break;

            // Also break if we've got to the end of an intermediate horizontal edge ...
            // nb: Smaller Dx's are to the right of larger Dx's ABOVE the horizontal.
            if (e->curr.x == edge->top.x && edge->next_in_LML && e->dx < edge->next_in_LML->dx)
                break;

            // note: may be done multiple times
            if (edge->index >= 0 && !is_open) {
                p1 = add_point(edge, e->curr, rings);
                edge_ptr<T> e_next_horizontal = sorted_edge_list;

                while (e_next_horizontal) {
                    if (e_next_horizontal->index >= 0 &&
                        horizontal_segments_overlap(edge->bot.x, edge->top.x,
                                                    e_next_horizontal->bot.x,
                                                    e_next_horizontal->top.x)) {
                        point_ptr<T> p2 = get_last_point(e_next_horizontal, rings);
                        joins.emplace_back(p2, p1, e_next_horizontal->top);
                    }
                    e_next_horizontal = e_next_horizontal->next_in_SEL;
                }
                ghost_joins.emplace_back(p1, nullptr, edge->bot);
            }

            // OK, so far we're still in range of the horizontal Edge  but make sure
            // we're at the last of consec. horizontals when matching with eMaxPair
            if (e == edge_max_pair && is_last_horizontal) {
                if (edge->index >= 0)
                    add_local_maximum_point(edge, edge_max_pair, edge->top, rings, active_edge_list);
                delete_from_AEL(edge, active_edge_list);
                delete_from_AEL(edge_max_pair, active_edge_list);
                return;
            }
            
            if (dir == left_to_right) {
                intersect_edges(edge, e, mapbox::geometry::point<T>(e->curr.x, edge->curr.y));
            } else {
                intersect_edges(e, edge, mapbox::geometry::point<T>(e->curr.x, edge->curr.y));
            }

            edge_ptr<T> e_next = get_next_in_AEL(e, dir);
            swap_positions_in_AEL(edge, e, active_edge_list);
            e = e_next;
        } // end while (e)

        // Break out of loop if HorzEdge.NextInLML is not also horizontal ...
        if (!edge->next_in_LML || !is_horizontal(edge->next_in_LML))
            break;

        update_edge_into_AEL(edge, active_edge_list, scanbeam);
        if (edge->index >= 0) add_point(edge, edge->bot, rings);
        get_horizontal_direction(edge, dir, left, right);
    } // end for (;;)

    if (edge->index >= 0 && !p1) {
        p1 = get_last_point(edge, rings);
        edge_ptr<T> e_next_horizontal = sorted_edge_list;
        while (e_next_horizontal)
        {
            if (e_next_horizontal->index >= 0 &&
                horizontal_segments_overlap(
                    edge->bot.X,
                    edge->top.X, 
                    e_next_horizontal->bot.X, 
                    e_next_horizontal->top.X
                )
            ) {
                point_ptr<T> p2 = get_last_point(e_next_horizontal, rings);
                joins.emplace_back(p2, p1, e_next_horizontal->top);
            }
            e_next_horizontal = e_next_horizontal->next_in_SEL;
        }
        ghost_joins.emplace_back(p1, nullptr, edge->top);
    }

    if (edge->next_in_LML) {
        if(edge->index >= 0) {
            p1 = add_point(edge, edge->top, rings);
            //When StrictlySimple and 'edge' is being touched by another edge, then
            //make sure both edges have a vertex here ...
            if (m_StrictSimple) {  
                edge_ptr<T> e_prev = edge->prev_in_AEL;
                edge_ptr<T> e_next = edge->next_in_AEL;

                if ((edge->winding_delta != 0) && e_prev && (e_prev->index >= 0) &&
                  (e_prev->curr.x == edge->top.x) && (e_prev->winding_delta != 0))
                {
                  //IntPoint pt = horzEdge->Top;
                  add_point(e_prev, edge->top, rings);
                }
                if ((edge->winding_delta != 0) && e_next && (e_next->index >= 0) &&
                    (e_next->curr.x == edge->top.x) && (e_next->winding_delta != 0))
                {
                  //IntPoint pt = horzEdge->Top;
                  AddOutPt(e_next, edge->top, rings);
                }
            }
            update_edge_into_AEL(edge, active_edge_list, scanbeam);

            if (edge->winding_delta == 0) return;

            //nb: HorzEdge is no longer horizontal here
            edge_ptr<T> e_prev = edge->prev_in_AEL;
            edge_ptr<T> e_next = edge->next_in_AEL;

            if (e_prev && e_prev->curr.x == edge->bot.x &&
                e_prev->curr.y == edge->bot.y && e_prev->winding_delta != 0 &&
                (e_prev->index >= 0 && e_prev->curr.y > e_prev->top.y &&
                slopes_equal(*edge, *e_prev)))
            {
                point<T> p2 = add_point(e_prev, edge->bot, rings);
                joins.emplace_back(p1, p2, edge->top);
            }
            else if (e_next && e_next->curr.x == edge->bot.x &&
                e_next->curr.y == edge->bot.y && e_next->winding_delta != 0 &&
                e_next->index >= 0 && e_next->curr.y > e_next->top.y &&
                slopes_equal(*edge, *e_next))
            {
                point<T> p2 = add_point(e_next, edge->bot, rings);
                joins.emplace_back(p1, p2, edge->top);
            }
        } else update_edge_into_AEL(edge, active_edge_list, scanbeam);
    } else {
        if (edge->index >= 0) add_point(edge, edge->top, rings);
        delete_from_AEL(edge, active_edge_list);
    }
}
}
}
}
