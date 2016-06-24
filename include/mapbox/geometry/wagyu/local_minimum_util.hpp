#pragma once

#include <sort>

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

// Make a list start on a local maximum by
// shifting all the points not on a local maximum to the
template <typename T>
void start_list_on_local_maximum(edge_list<T>& edges) {
    if (edges.size() <= 2) {
        return;
    }
    // Find the first local maximum going forward in the list
    auto prev_edge = edges.end();
    --prev_edge;
    bool prev_edge_is_horizontal = is_horizontal(*prev_edge);
    auto edge = edges.begin();
    bool edge_is_horizontal;
    bool y_decreasing_before_last_horizontal = false; // assume false at start

    while (edge != edges.end()) {
        edge_is_horizontal = is_horizontal(*edge);
        if ((!prev_edge_is_horizontal && !edge_is_horizontal && edge->top == prev_edge->top)) {
            break;
        }
        if (!edge_is_horizontal && prev_edge_is_horizontal) {
            if (y_decreasing_before_last_horizontal &&
                (edge->top == prev_edge->bot || edge->top == prev_edge->top)) {
                break;
            }
        } else if (!y_decreasing_before_last_horizontal && !prev_edge_is_horizontal &&
                   edge_is_horizontal &&
                   (prev_edge->top == edge->top || prev_edge->top == edge->bot)) {
            y_decreasing_before_last_horizontal = true;
        }
        prev_edge_is_horizontal = edge_is_horizontal;
        prev_edge = edge;
        ++edge;
    }
    if (edge != edges.end() && edge != edges.begin()) {
        edges.splice(edges.end(), edges, edges.begin(), edge);
    } else if (edges.begin()->top.y > prev_edge->bot.y) {
        // This should only happen in lines not in rings
        std::reverse(edges.begin(), edges.end());
    }
}

template <typename T>
bound<T> create_bound_towards_minimum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        if (is_horizontal(edges.front())) {
            reverse_horizontal(edges.front());
        }
        bound<T> bnd;
        bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), edges.end());
        return bnd;
    }

    auto next_edge = edges.begin();
    auto edge = next_edge;
    ++next_edge;
    bool edge_is_horizontal = is_horizontal(*edge);
    if (edge_is_horizontal) {
        reverse_horizontal(*edge);
    }
    bool next_edge_is_horizontal;
    bool y_increasing_before_last_horizontal = false; // assume false at start

    while (next_edge != edges.end()) {
        next_edge_is_horizontal = is_horizontal(*next_edge);
        if ((!next_edge_is_horizontal && !edge_is_horizontal && edge->bot == next_edge->bot)) {
            break;
        }
        if (!next_edge_is_horizontal && edge_is_horizontal) {
            if (y_increasing_before_last_horizontal &&
                (next_edge->bot == edge->bot || next_edge->bot == edge->top)) {
                break;
            }
        } else if (!y_increasing_before_last_horizontal && !edge_is_horizontal &&
                   next_edge_is_horizontal &&
                   (edge->bot == next_edge->top || edge->bot == next_edge->bot)) {
            y_increasing_before_last_horizontal = true;
        }
        edge_is_horizontal = next_edge_is_horizontal;
        edge = next_edge;
        if (edge_is_horizontal) {
            reverse_horizontal(*edge);
        }
        ++next_edge;
    }
    bound<T> bnd;
    bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), next_edge);
    std::reverse(bnd.edges.begin(), bnd.edges.end());
    return bnd;
}

template <typename T>
bound<T> create_bound_towards_maximum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        bound<T> bnd;
        bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), edges.end());
        return bnd;
    }
    auto next_edge = edges.begin();
    auto edge = next_edge;
    ++next_edge;
    bool edge_is_horizontal = is_horizontal(*edge);
    bool next_edge_is_horizontal;
    bool y_decreasing_before_last_horizontal = false; // assume false at start

    while (next_edge != edges.end()) {
        next_edge_is_horizontal = is_horizontal(*next_edge);
        if ((!next_edge_is_horizontal && !edge_is_horizontal && edge->top == next_edge->top)) {
            break;
        }
        if (!next_edge_is_horizontal && edge_is_horizontal) {
            if (y_decreasing_before_last_horizontal &&
                (next_edge->top == edge->bot || next_edge->top == edge->top)) {
                break;
            }
        } else if (!y_decreasing_before_last_horizontal && !edge_is_horizontal &&
                   next_edge_is_horizontal &&
                   (edge->top == next_edge->top || edge->top == next_edge->bot)) {
            y_decreasing_before_last_horizontal = true;
        }
        edge_is_horizontal = next_edge_is_horizontal;
        edge = next_edge;
        ++next_edge;
    }
    edge_list<T> bnd;
    bnd.edges.splice(bnd.edges.end(), edges, edges.begin(), next_edge);
    return bnd;
}

template <typename T>
void set_edge_data(edge_list<T>& edges, bound_ptr<T> bound) {
    for (auto& e : edges) {
        e.bound = bound;
        e.curr = e.bot;
    }
}

template <typename T>
void move_horizontals_on_left_to_right(bound<T>& left_bound, bound<T>& right_bound) {
    // We want all the horizontal segments that are at the same Y as the minimum to be on the right
    // bound
    for (auto edge_itr = left_bound.edges.begin(); edge_itr != left_bound.edges.end(); ++edge_itr) {
        if (!is_horizontal(*edge_itr)) {
            break;
        } else {
            reverse_horizontal(*edge_itr);
        }
    }
    if (edge_itr == left_bound.edges.begin()) {
        return;
    }
    auto original_first = right_bound.edges.begin();
    right_bound.edges.splice(original_first, left_bound.edges, left_bound.edges.begin(), edge_itr);
    std::reverse(right_bound.edges.begin(), original_first);
}

template <typename T>
void add_line_to_local_minima_list(edge_list<T>& edges, local_minimum_list<T>& minima_list) {
    using value_type = T;

    if (edges.empty()) {
        return;
    }
    // Adjust the order of the ring so we start on a local maximum
    // therefore we start right away on a bound.
    start_list_on_local_maximum(edges);
    edge_ptr<T> last_maximum = nullptr;
    while (!edges.empty()) {
        auto to_minimum = create_bound_towards_minimum(edges);
        assert(!to_minimum.edges.empty());
        auto const& min_front = to_minimum.edges.front();
        to_minimum.poly_type = polygon_type_subject;
        set_edge_data(to_minimum.edges, &to_minimum);
        to_minimum.maximum_edge_pair = last_maximum;
        to_minimum.winding_delta = 0;
        if (edges.empty()) {
            if (min_font.dx < 0.0) {
                to_minimum.side = edge_left;
                bound<T> right_bound;
                right_bound.winding_delta = 0;
                right_bound.side = edge_right;
                right_bound.poly_type = polygon_type_subject;
                move_horizontals_on_left_to_right(to_minimum, right_bound);
                if (!right_bound.empty()) {
                    set_edge_data(right_bound.edges, &right_bound);
                }
                minima_list.emplace_back(std::move(to_minimum), std::move(right_bound),
                                         min_front.y);
            } else {
                to_minimum.side = edge_right;
                minima_list.emplace_back(bound<T>(), std::move(to_minimum), min_front.y);
            }
            break;
        }
        auto to_maximum = create_bound_towards_maximum(edges);
        assert(!to_maximum.edges.empty());
        auto const& max_front = to_maximum.edges.front();
        to_maximum.maximum_edge_pair = edges.empty() ? nullptr : &(edges.front());
        last_maximum = &(to_maximum.edges.back());
        to_maximum.poly_type = polygon_type_subject;
        to_maximum.winding_delta = 0;
        set_edge_data(to_maximum.edges, &to_maximum);
        if (max_front.dx < min_front.dx) {
            to_minimum.side = edge_right;
            to_maximum.side = edge_left;
            minima_list.emplace_back(std::move(to_maximum), std::move(to_minimum), min_front.bot.y);
        } else {
            to_minimum.side = edge_left;
            to_maximum.side = edge_right;
            minima_list.emplace_back(std::move(to_minimum), std::move(to_maximum), min_front.bot.y);
        }
    }
}

template <typename T>
void add_ring_to_local_minima_list(edge_list<T>& edges,
                                   local_minimum_list<T>& minima_list,
                                   polygon_type poly_type) {
    using value_type = T;

    if (edges.empty()) {
        return;
    }
    // Adjust the order of the ring so we start on a local maximum
    // therefore we start right away on a bound.
    start_list_on_local_maximum(edges);

    edge_ptr<T> first_maximum = &(edges.front());
    edge_ptr<T> last_maximum = first_maximum;
    while (!edges.empty()) {
        auto to_minimum = create_bound_towards_minimum(edges);
        assert(!edges.empty());
        auto to_maximum = create_bound_towards_maximum(edges);
        if (to_maximum.edges.front().dx < to_minimum.edges.front().dx) {
            move_horizontals_on_left_to_right(to_maximum, to_minimum);
        } else {
            move_horizontals_on_left_to_right(to_minimum, to_maximum);
        }
        assert(!to_minimum.edges.empty());
        assert(!to_maximum.edges.empty());
        auto const& min_front = to_minimum.edges.front();
        auto const& max_front = to_maximum.edges.front();
        to_minimum.maximum_edge_pair = last_maximum;
        to_maximum.maximum_edge_pair = edges.empty() ? first_maximum : &(edges.front());
        last_maximum = &(to_maximum.edges.back());
        to_minimum.poly_type = poly_type;
        to_maximum.poly_type = poly_type;
        set_edge_data(to_minimum.edges, &to_minimum);
        set_edge_data(to_maximum.edges, &to_maximum);
        if (max_front.dx < min_front.dx) {
            to_minimum.side = edge_right;
            to_maximum.side = edge_left;
            to_minimum.winding_delta = 1;
            to_maximum.winding_delta = -1;
            minima_list.emplace_back(std::move(to_maximum), std::move(to_minimum), min_front.bot.y);
        } else {
            to_minimum.side = edge_left;
            to_maximum.side = edge_right;
            to_minimum.winding_delta = -1;
            to_maximum.winding_delta = 1;
            minima_list.emplace_back(std::move(to_minimum), std::move(to_maximum), min_front.bot.y);
        }
    }
}
}
}
}
