#pragma once

#include <mapbox/geometry/wagy/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/bound.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct sorting_bound {

    active_bound_list_itr<T> bound;
    std::size_t index;

    sorting_bound(active_bound_list_itr<T> bound_, std::size_t index_)
        : bound(bound_), index(index_) {
    }
}

template <typename T>
using sorting_bound_list = std::list<sorting_bound<T>>;

template <typename T>
using sorting_bound_list_itr = typename sorting_bound_list<T>::iterator;

template <typename T>
struct sorting_bound_sorter {
    inline bool operator()(sorting_bound<T> const& bound1, sorting_bound<T> const& bound2) {
        return bound2.index < bound1.index;
    }
};
}
}
}
