#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct local_minimum;

template <typename T>
using local_minimum_ptr = local_minimum<T>*;

template <typename T>
using const_local_minimum_ptr = local_minimum<T>* const;

template <typename T>
struct local_minimum {
    edge_list<T> left_bound;
    edge_list<T> right_bound;
    T y;

    local_minimum(T y_, edge_list<T>&& left_bound_, edge_list<T>&& right_bound_)
        : left_bound(left_bound_), right_bound(right_bound_), y(y_) {
    }
};

template <typename T>
using local_minimum_list = std::vector<local_minimum<T>>;

template <typename T>
using local_minimum_itr = typename local_minimum_list<T>::iterator;
}
}
}
