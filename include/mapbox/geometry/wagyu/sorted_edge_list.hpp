#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct sorting_edge {

    edge_list_itr<T> edge;
    std::size_t index;

    sorting_edge(edge_list_itr<T> edge_, std::size_t index_) : edge(edge_), index(index_) {
    }
}

template <typename T>
using sorting_edge_list = std::list<sorting_edge<T>>;

template <typename T>
using sorting_edge_list_itr = typename sorting_edge_list<T>::iterator;

template <typename T>
struct sorting_edge_sorter {
    inline bool operator()(sorting_edge<T> const& edge1, sorting_edge<T> const& edge2) {
        return edge2.index < edge1.index;
    }
};

template <typename T>
inline bool is_maxima(edge_ptr<T> e, T y) {
    return e && e->top.y == y && !e->next_in_LML;
}

template <typename T>
inline bool is_intermediate(edge_ptr<T> e, T y) {
    return e && e->top.y == y && e->next_in_LML;
}
}
}
}
