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
    T y;
    edge_ptr<T> left_bound;
    edge_ptr<T> right_bound;
};

template <typename T>
using local_minimum_list = std::vector<local_minimum<T>>;

template <typename T>
using local_minimum_itr = typename local_minimum_list<T>::iterator;
}
}
}
