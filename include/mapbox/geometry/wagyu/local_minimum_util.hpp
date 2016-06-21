#pragma once

#include <sort>

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct local_minimum_sorter {
    inline bool operator()(local_minimum<T> const& locMin1, local_minimum<T> const& locMin2) {
        return locMin2.y < locMin1.y;
    }
};

template <typename T>
void sort_local_minima(local_minum_list<T> & minima_list) {
    std::stable_sort(minima_list.begin(), minima_list.end(), local_minimum_sorter<value_type>());
}

template <typename T>
bool pop_local_minima(T Y,
                      local_minimum_itr<T>& current_local_minimum,
                      local_minimum_list<T>& minima_list) {
    if (current_local_minimum == minima_list.end() || (*current_local_minimum).y != Y) {
        return false;
    }
    ++current_local_minimum;
    return true;
}

template <typename T>
bool local_minima_pending(local_minimum_itr<T>& current_local_minimum,
                          local_minimum_list<T>& minima_list) {
    return (current_local_minimum != minima_list.end());
}

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
edge_list<T> create_bound_towards_minimum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        if (is_horizontal(edges.front())) {
            reverse_horizontal(edges.front());
        }
        edge_list<T> bound;
        bound.splice(bound.end(), edges, edges.begin(), edges.end());
        return bound;
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
    edge_list<T> bound;
    bound.splice(bound.end(), edges, edges.begin(), next_edge);
    std::reverse(bound.begin(), bound.end());
    return bound;
}

template <typename T>
edge_list<T> create_bound_towards_maximum(edge_list<T>& edges) {
    if (edges.size() == 1) {
        edge_list<T> bound;
        bound.splice(bound.end(), edges, edges.begin(), edges.end());
        return bound;
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
    edge_list<T> bound;
    bound.splice(bound.end(), edges, edges.begin(), next_edge);
    return bound;
}

template <typename T>
void set_edge_data(edge_list<T>& edges, std::int8_t winding_value, edge_side side) {
    for (auto& e : edges) {
        e.side = side;
        e.curr = e.bot;
        e.winding_delta = winding_value;
    }
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

    while (!edges.empty()) {
        auto to_minimum = create_bound_towards_minimum(edges);
        assert(!to_minimum.empty());
        if (edges.empty()) {
            value_type y = to_minimum_begin->bot.y;
            minima_list.emplace_back(y, edge_list<value_type>(), std::move(to_minimum));
        }
        auto to_maximum = create_bound_towards_maximum(edges);
        assert(!to_maximum.empty());
        auto to_minimum_begin = to_minimum.begin();
        auto to_maximum_begin = to_minimum.begin();
        value_type y = to_minimum_begin->bot.y;
        if (to_minimum_begin->dx < to_maximum_begin->dx) {
            minima_list.emplace_back(y, std::move(to_maximum), std::move(to_minimum));
        } else {
            minima_list.emplace_back(y, std::move(to_minimum), std::move(to_maximum));
        }
    }
}

template <typename T>
void add_ring_to_local_minima_list(edge_list<T>& edges, local_minimum_list<T>& minima_list) {
    using value_type = T;

    if (edges.empty()) {
        return;
    }
    // Adjust the order of the ring so we start on a local maximum
    // therefore we start right away on a bound.
    start_list_on_local_maximum(edges);

    while (!edges.empty()) {
        auto to_minimum = create_bound_towards_minimum(edges);
        assert(!edges.empty());
        auto to_maximum = create_bound_towards_maximum(edges);
        assert(!to_minimum.empty());
        assert(!to_maximum.empty());
        auto to_minimum_begin = to_minimum.begin();
        auto to_maximum_begin = to_minimum.begin();
        value_type y = to_minimum_begin->bot.y;
        if (to_minimum_begin->dx < to_maximum_begin->dx) {
            set_edge_data(to_minimum, 1, edge_right);
            set_edge_data(to_maximum, -1, edge_left);
            minima_list.emplace_back(y, std::move(to_maximum), std::move(to_minimum));
        } else {
            set_edge_data(to_minimum, 1, edge_left);
            set_edge_data(to_maximum, -1, edge_right);
            minima_list.emplace_back(y, std::move(to_minimum), std::move(to_maximum));
        }
    }
}

}
}
}
