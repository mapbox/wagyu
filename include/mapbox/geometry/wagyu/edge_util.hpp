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
inline void swap_ring_indexes(edge<T>& Edge1, edge<T>& Edge2) {
    std::size_t index = Edge1.index;
    Edge1.index = Edge2.index;
    Edge2.index = index;
}

template <typename T>
inline void swap_sides(edge<T>& Edge1, edge<T>& Edge2) {
    edge_side side = Edge1.side;
    Edge1.side = Edge2.side;
    Edge2.side = side;
}

template <typename T>
inline void reverse_horizontal(edge<T>& e) {
    // swap horizontal edges' top and bottom x's so they follow the natural
    // progression of the bounds - ie so their xbots will align with the
    // adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
    std::swap(e.top.x, e.bot.x);
}

template <typename T>
edge_ptr<T> process_bound_type_line(edge_ptr<T> current_edge,
                                    bool next_is_forward,
                                    local_minimum_list<T>& minima_list) {
    edge_ptr<T> result = current_edge;
    edge_ptr<T> horizontal_edge = nullptr;

    if (current_edge->index == EDGE_SKIP) {
        // if edges still remain in the current bound beyond the skip edge then
        // create another LocMin and call ProcessBound once more
        if (next_is_forward) {
            while (current_edge->top.y == current_edge->next->bot.y) {
                current_edge = current_edge->next;
            }
            // don't include top horizontals when parsing a bound a second time,
            // they will be contained in the opposite bound ...
            while (current_edge != result && is_horizontal(*current_edge)) {
                current_edge = current_edge->prev;
            }
        } else {
            while (current_edge->top.y == current_edge->prev->bot.y) {
                current_edge = current_edge->prev;
            }
            while (current_edge != result && is_horizontal(*current_edge)) {
                current_edge = current_edge->next;
            }
        }

        if (current_edge == result) {
            if (next_is_forward) {
                result = current_edge->next;
            } else {
                result = current_edge->prev;
            }
        } else {
            // there are more edges in the bound beyond result starting with
            // current_edge
            if (next_is_forward) {
                current_edge = result->next;
            } else {
                current_edge = result->prev;
            }
            local_minimum<T> locMin;
            locMin.y = current_edge->bot.y;
            locMin.left_bound = nullptr;
            locMin.right_bound = current_edge;
            current_edge->winding_delta = 0;
            result = process_bound_type_line(current_edge, next_is_forward, minima_list);
            minima_list.push_back(locMin);
        }
        return result;
    }

    edge_ptr<T> starting_edge = nullptr;

    if (is_horizontal(*current_edge)) {
        // We need to be careful with open paths because this may not be a
        // true local minima (ie current_edge may be following a skip edge).
        // Also, consecutive horz. edges may start heading left before going
        // right.
        if (next_is_forward) {
            starting_edge = current_edge->prev;
        } else {
            starting_edge = current_edge->next;
        }
        if (is_horizontal(*starting_edge)) // ie an adjoining horizontal skip edge
        {
            if (starting_edge->bot.x != current_edge->bot.x &&
                starting_edge->top.x != current_edge->bot.x) {
                reverse_horizontal(*current_edge);
            }
        } else if (starting_edge->bot.x != current_edge->bot.x) {
            reverse_horizontal(*current_edge);
        }
    }

    starting_edge = current_edge;
    if (next_is_forward) {
        while (result->top.y == result->next->bot.y && result->next->index != EDGE_SKIP) {
            result = result->next;
        }
        if (is_horizontal(*result) && result->next->index != EDGE_SKIP) {
            // nb: at the top of a bound, horizontals are added to the bound
            // only when the preceding edge attaches to the horizontal's left
            // vertex
            // unless a Skip edge is encountered when that becomes the top
            // divide
            horizontal_edge = result;
            while (is_horizontal(*horizontal_edge->prev)) {
                horizontal_edge = horizontal_edge->prev;
            }
            if (horizontal_edge->prev->top.x > result->next->top.x) {
                result = horizontal_edge->prev;
            }
        }
        while (current_edge != result) {
            current_edge->next_in_LML = current_edge->next;
            if (is_horizontal(*current_edge) && current_edge != starting_edge &&
                current_edge->bot.x != current_edge->prev->top.x) {
                reverse_horizontal(*current_edge);
            }
            current_edge = current_edge->next;
        }
        if (is_horizontal(*current_edge) && current_edge != starting_edge &&
            current_edge->bot.x != current_edge->prev->top.x) {
            reverse_horizontal(*current_edge);
        }
        result = result->next; // move to the edge just beyond current bound
    } else {
        while (result->top.y == result->prev->bot.y && result->prev->index != EDGE_SKIP) {
            result = result->prev;
        }
        if (is_horizontal(*result) && result->prev->index != EDGE_SKIP) {
            horizontal_edge = result;
            while (is_horizontal(*horizontal_edge->next)) {
                horizontal_edge = horizontal_edge->next;
            }
            if (horizontal_edge->next->top.x >= result->prev->top.x) {
                result = horizontal_edge->next;
            }
        }

        while (current_edge != result) {
            current_edge->next_in_LML = current_edge->prev;
            if (is_horizontal(*current_edge) && current_edge != starting_edge &&
                current_edge->bot.x != current_edge->next->top.x) {
                reverse_horizontal(*current_edge);
            }
            current_edge = current_edge->prev;
        }
        if (is_horizontal(*current_edge) && current_edge != starting_edge &&
            current_edge->bot.x != current_edge->next->top.x) {
            reverse_horizontal(*current_edge);
        }
        result = result->prev; // move to the edge just beyond current bound
    }
    return result;
}

template <typename T>
edge_ptr<T> process_bound_type_ring(edge_ptr<T> current_edge, bool next_is_forward) {
    edge_ptr<T> result = current_edge;
    edge_ptr<T> horizontal_edge = nullptr;
    edge_ptr<T> starting_edge = nullptr;

    if (is_horizontal(*current_edge)) {
        // We need to be careful with open paths because this may not be a
        // true local minima (ie E may be following a skip edge).
        // Also, consecutive horz. edges may start heading left before going
        // right.
        if (next_is_forward) {
            starting_edge = current_edge->prev;
        } else {
            starting_edge = current_edge->next;
        }
        if (is_horizontal(*starting_edge)) // ie an adjoining horizontal skip edge
        {
            if (starting_edge->bot.x != current_edge->bot.x &&
                starting_edge->top.x != current_edge->bot.x) {
                reverse_horizontal(*current_edge);
            }
        } else if (starting_edge->bot.x != current_edge->bot.x) {
            reverse_horizontal(*current_edge);
        }
    }

    starting_edge = current_edge;
    if (next_is_forward) {
        while (result->top.y == result->next->bot.y) {
            result = result->next;
        }
        if (is_horizontal(*result)) {
            // nb: at the top of a bound, horizontals are added to the bound
            // only when the preceding edge attaches to the horizontal's left
            // vertex
            // unless a Skip edge is encountered when that becomes the top
            // divide
            horizontal_edge = result;
            while (is_horizontal(*horizontal_edge->prev)) {
                horizontal_edge = horizontal_edge->prev;
            }
            if (horizontal_edge->prev->top.x > result->next->top.x) {
                result = horizontal_edge->prev;
            }
        }
        while (current_edge != result) {
            current_edge->next_in_LML = current_edge->next;
            if (is_horizontal(*current_edge) && current_edge != starting_edge &&
                current_edge->bot.x != current_edge->prev->top.x) {
                reverse_horizontal(*current_edge);
            }
            current_edge = current_edge->next;
        }
        if (is_horizontal(*current_edge) && current_edge != starting_edge &&
            current_edge->bot.x != current_edge->prev->top.x) {
            reverse_horizontal(*current_edge);
        }
        result = result->next; // move to the edge just beyond current bound
    } else {
        while (result->top.y == result->prev->bot.y) {
            result = result->prev;
        }
        if (is_horizontal(*result)) {
            horizontal_edge = result;
            while (is_horizontal(*horizontal_edge->next)) {
                horizontal_edge = horizontal_edge->next;
            }
            if (horizontal_edge->next->top.x >= result->prev->top.x) {
                result = horizontal_edge->next;
            }
        }

        while (current_edge != result) {
            current_edge->next_in_LML = current_edge->prev;
            if (is_horizontal(*current_edge) && current_edge != starting_edge &&
                current_edge->bot.x != current_edge->next->top.x) {
                reverse_horizontal(*current_edge);
            }
            current_edge = current_edge->prev;
        }
        if (is_horizontal(*current_edge) && current_edge != starting_edge &&
            current_edge->bot.x != current_edge->next->top.x) {
            reverse_horizontal(*current_edge);
        }
        result = result->prev; // move to the edge just beyond current bound
    }
    return result;
}

template <typename T>
edge_ptr<T> find_next_local_minimum(edge_ptr<T> edge) {
    while (true) {
        while (edge->bot != edge->prev->bot || edge->curr == edge->top) {
            edge = edge->next;
        }
        if (!is_horizontal(*edge) && !is_horizontal(*edge->prev)) {
            break;
        }
        while (is_horizontal(*edge->prev)) {
            edge = edge->prev;
        }
        edge_ptr<T> edge2 = edge;
        while (is_horizontal(*edge)) {
            edge = edge->next;
        }
        if (edge->top.y == edge->prev->bot.y) {
            continue; // ie just an intermediate horz.
        }
        if (edge2->prev->bot.x < edge->bot.x) {
            edge = edge2;
        }
        break;
    }
    return edge;
}

template <typename T>
void add_flat_line_to_local_minima_list(edge_list<T>& new_edges,
                                        local_minimum_list<T>& minima_list) {
    using value_type = T;
    // Totally flat paths must be handled differently when adding them
    // to LocalMinima list to avoid endless loops etc ...
    edge_ptr<value_type> current_edge = &new_edges.back();
    current_edge->prev->index = EDGE_SKIP;
    local_minimum<value_type> local_min;
    local_min.y = current_edge->bot.y;
    local_min.left_bound = nullptr;
    local_min.right_bound = current_edge;
    local_min.right_bound->side = edge_right;
    local_min.right_bound->winding_delta = 0;
    for (;;) {
        if (current_edge->bot.x != current_edge->prev->top.x) {
            reverse_horizontal(*current_edge);
        }
        if (current_edge->next->index == EDGE_SKIP) {
            break;
        }
        current_edge->next_in_LML = current_edge->next;
        current_edge = current_edge->next;
    }
    minima_list.push_back(local_min);
}

template <typename T>
void add_line_to_local_minima_list(edge_list<T>& new_edges, local_minimum_list<T>& minima_list) {
    using value_type = T;
    bool left_bound_is_forward = false;
    edge_ptr<value_type> starting_edge = &new_edges.back();
    edge_ptr<value_type> current_edge = starting_edge;
    edge_ptr<value_type> minimum_edge = nullptr;

    for (;;) {
        current_edge = find_next_local_minimum(current_edge);
        if (current_edge == minimum_edge) {
            break;
        } else if (minimum_edge == nullptr) {
            minimum_edge = current_edge;
        }

        // E and E.prev now share a local minima (left aligned if horizontal).
        // Compare their slopes to find which starts which bound ...
        local_minimum<value_type> local_min;
        local_min.y = current_edge->bot.y;
        if (current_edge->dx < current_edge->prev->dx) {
            local_min.left_bound = current_edge->prev;
            local_min.right_bound = current_edge;
            left_bound_is_forward = false; // Q.next_in_LML = Q.prev
        } else {
            local_min.left_bound = current_edge;
            local_min.right_bound = current_edge->prev;
            left_bound_is_forward = true; // Q.next_in_LML = Q.next
        }

        local_min.left_bound->winding_delta = 0;
        local_min.right_bound->winding_delta = 0;

        current_edge =
            process_bound_type_line(local_min.left_bound, left_bound_is_forward, minima_list);
        if (current_edge->index == EDGE_SKIP) {
            current_edge =
                process_bound_type_line(current_edge, left_bound_is_forward, minima_list);
        }

        edge_ptr<value_type> current_edge_2 =
            process_bound_type_line(local_min.right_bound, !left_bound_is_forward, minima_list);
        if (current_edge_2->index == EDGE_SKIP) {
            current_edge_2 =
                process_bound_type_line(current_edge_2, !left_bound_is_forward, minima_list);
        }

        minima_list.push_back(local_min);

        if (!left_bound_is_forward) {
            current_edge = current_edge_2;
        }
    }
}

template <typename T>
void add_ring_to_local_minima_list(edge_list<T>& new_edges, local_minimum_list<T>& minima_list) {
    using value_type = T;
    bool left_bound_is_forward = false;
    edge_ptr<value_type> starting_edge = &new_edges.back();
    edge_ptr<value_type> current_edge = starting_edge;
    edge_ptr<value_type> minimum_edge = nullptr;

    for (;;) {
        current_edge = find_next_local_minimum(current_edge);
        if (current_edge == minimum_edge) {
            break;
        } else if (minimum_edge == nullptr) {
            minimum_edge = current_edge;
        }

        // E and E.prev now share a local minima (left aligned if horizontal).
        // Compare their slopes to find which starts which bound ...
        local_minimum<value_type> local_min;
        local_min.y = current_edge->bot.y;
        if (current_edge->dx < current_edge->prev->dx) {
            local_min.left_bound = current_edge->prev;
            local_min.right_bound = current_edge;
            left_bound_is_forward = false; // Q.next_in_LML = Q.prev
        } else {
            local_min.left_bound = current_edge;
            local_min.right_bound = current_edge->prev;
            left_bound_is_forward = true; // Q.next_in_LML = Q.next
        }

        if (local_min.left_bound->next == local_min.right_bound) {
            local_min.left_bound->winding_delta = -1;
            local_min.right_bound->winding_delta = 1;
        } else {
            local_min.left_bound->winding_delta = 1;
            local_min.right_bound->winding_delta = -1;
        }

        current_edge = process_bound_type_ring(local_min.left_bound, left_bound_is_forward);

        edge_ptr<value_type> current_edge_2 =
            process_bound_type_ring(local_min.right_bound, !left_bound_is_forward);

        minima_list.push_back(local_min);

        if (!left_bound_is_forward) {
            current_edge = current_edge_2;
        }
    }
}

template <typename T>
void make_list_circular(edge_list<T>& edges) {
    // Link all edges for circular list now
    edges.front().prev = &edges.back();
    edges.back().next = &edges.front();
    auto itr_next = edges.begin();
    auto itr = itr_next++;
    for (; itr_next != edges.end(); ++itr, ++itr_next) {
        itr->next = &(*itr_next);
        itr_next->prev = &(*itr);
    }
}

template <typename T>
bool build_edge_list(mapbox::geometry::line_string<T> const& path_geometry,
                     edge_list<T>& edges,
                     bool& is_flat) {
    if (path_geometry.size() < 2) {
        return false;
    }

    auto itr_next = path_geometry.begin();
    ++itr_next;
    auto itr = path_geometry.begin();
    while (itr_next != path_geometry.end()) {
        if (*itr_next == *itr) {
            // Duplicate point advance itr_next, but do not
            // advance itr
            ++itr_next;
            continue;
        }

        if (is_flat && itr_next->y != itr->y) {
            is_flat = false;
        }
        edges.emplace_back(*itr, *itr_next, polygon_type_subject);
        itr = itr_next;
        ++itr_next;
    }

    if (edges.size() < 2) {
        return false;
    }

    return true;
}

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

    make_list_circular(new_edges);

    if (is_flat) {
        add_flat_line_to_local_minima_list(new_edges, minima_list);
    } else {
        add_line_to_local_minima_list(new_edges, minima_list);
    }
    return true;
}

template <typename T>
bool build_edge_list(mapbox::geometry::linear_ring<T> const& path_geometry,
                     edge_list<T>& edges,
                     polygon_type p_type) {
    using value_type = T;

    if (path_geometry.size() < 3) {
        return false;
    }

    // As this is a loop, we need to first go backwards from end to try and find
    // the proper starting point for the iterators before the beginning

    auto itr_rev = path_geometry.end();
    auto itr = path_geometry.begin();
    --itr_rev;
    mapbox::geometry::point<value_type> pt1 = *itr_rev;
    mapbox::geometry::point<value_type> pt2 = *itr;
    mapbox::geometry::point<value_type> pt3 = *itr;

    // Find next non repeated point going backwards from
    // end for pt1
    while (pt1 == pt2) {
        pt1 = *(--itr_rev);
        if (itr_rev == path_geometry.begin()) {
            return false;
        }
    }

    mapbox::geometry::point<value_type> pt_first = pt1;

    while (true) {
        if (pt3 == pt2) {
            // Duplicate point advance itr, but do not
            // advance other points
            ++itr;
            if (itr == path_geometry.end()) {
                break;
            }
            pt3 = *itr;
            continue;
        }

        // Now check if slopes are equal between two segments - either
        // a spike or a collinear point - if so drop point number 2.
        if (slopes_equal(pt1, pt2, pt3)) {
            // We need to reconsider previously added points
            // because the point it was using was found to be collinear
            // or a spike
            pt2 = pt1;
            if (!edges.empty()) {
                edges.pop_back(); // remove previous edge (pt1)
            }
            if (!edges.empty()) {
                pt1 = edges.back().curr;
            } else {
                pt1 = pt_first;
            }
            continue;
        }

        edges.emplace_back(pt2, pt3, p_type);
        ++itr;
        pt1 = pt2;
        pt2 = pt3;
        pt3 = *itr;
        if (itr == path_geometry.end()) {
            break;
        }
    }

    // If the geometry does not explicity close the geometry
    // we still need to add one more segment.
    if (path_geometry.front() != path_geometry.back()) {
        pt3 = path_geometry.front();
        // Now check if slopes are equal between two segments - either
        // a spike or a collinear point - if so drop point number 2.
        while (slopes_equal(pt1, pt2, pt3)) {
            // We need to reconsider previously added points
            // because the point it was using was found to be collinear
            // or a spike
            pt2 = pt1;
            if (!edges.empty()) {
                edges.pop_back(); // remove previous edge (pt1)
            } else {
                pt1 = pt_first;
                break;
            }
            if (!edges.empty()) {
                pt1 = edges.back().curr;
            } else {
                pt1 = pt_first;
                break;
            }
        }
        edges.emplace_back(pt2, pt3, p_type);
    }

    if (edges.size() < 3) {
        return false;
    }
    return true;
}

template <typename T>
bool add_linear_ring(mapbox::geometry::linear_ring<T> const& path_geometry,
                     std::vector<edge_list<T>>& edges,
                     local_minimum_list<T>& minima_list,
                     polygon_type p_type) {
    edges.emplace_back();
    auto& new_edges = edges.back();
    if (!build_edge_list(path_geometry, new_edges, p_type) || new_edges.empty()) {
        edges.pop_back();
        return false;
    }
    make_list_circular(new_edges);
    add_ring_to_local_minima_list(new_edges, minima_list);
    return true;
}

template <typename T>
void process_edges_at_top_of_scanbeam(T top_y,
                                      edge_ptr<T> active_edges,
                                      std::list<T>& maxima,
                                      local_minimum_list<T>& minima_list,
                                      local_minimum_itr<T>& current_lm,
                                      bool use_full_range) {
    std::list<T> next_maxima;
    edge_ptr<T>* e = active_edges;
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
            edge<T> e_prev = e->prev_in_AEL;
            do_maxima(e);
            if (!e_prev) {
                e = active_edges;
            } else {
                e = e_prev->next_in_AEL;
            }
        } else {
            // 2. Promote horizontal edges. If not horizontal, update curr.x and curr.y.

            if (is_intermediate(e, top_y) && is_horizontal(*e->next_in_LML)) {
                update_edge_into_AEL(e);
                if (e->out_index == 0) {
                    add_out_point(e, e->bot);
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

        if (e->out_index >= 0 && e->wind_delta != 0) {
            edge<T> e_prev = e->prev_in_AEL;
            while (e_prev && e->prev->curr.x == e->curr.x) {
                if (e_prev->out_index >= 0 && e_prev->wind_delta != 0 &&
                    !(e->bot == e_prev->bot && e->top == e_prev->top)) {
                    mapbox::geometry::point<T> pt = e->curr;
                    point<T> op = add_out_point(e_prev, pt);
                    point<T> op2 = add_out_point(e, pt);
                    add_join(op, op2, pt); // strictly simple type 3 join
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
            if (e->out_index >= 0) {
                op = add_out_point(e, e->top);
            }
            update_edge_into_AEL(e);

            // If output polygons share an edge, they'll need to be joined later.

            edge_ptr<T> e_prev = e->prev_in_AEL;
            edge_ptr<T> e_next = e->next_in_AEL;

            if (e_prev && e_prev->curr.x == e->bot.x && e_prev->out_index >= 0 &&
                e_prev->curr.y > e_prev->top.y &&
                slopes_equal(e->curr, e->top, e_prev->curr, e_prev->top, use_full_range) &&
                (e->wind_delta != 0) && (e_prev->wind_delta != 0)) {
                point_ptr<T> op2 = add_out_point(e_prev, e->bot);
                add_join(op, op2, e->top);
            } else if (e_next && e_next->curr.x == e->bot.x && e_next->curr.y == e->bot.y &&
                       e->next->out_index >= 0 && e_next->curr.y > e_next->top.y &&
                       slopes_equal(e->curr, e->top, e_next->curr, e_next->top, use_full_range) &&
                       (e->wind_delta != 0) && (e_next->wind_delta != 0)) {
                point_ptr<T> op2 = add_out_point(e_next, e->bot);
                add_join(op, op2, e->top);
            }
        }
        e = e->next_in_AEL;
    }
}
}
}
}
