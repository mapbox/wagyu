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

template <typename T>
struct local_minimum_sorter {
    inline bool operator()(local_minimum<T> const& locMin1, local_minimum<T> const& locMin2) {
        return locMin2.y < locMin1.y;
    }
};

template <typename T>
bool pop_local_minima(T Y,
                      local_minimum_ptr<T>& local_min,
                      local_minimum_itr<T>& current_local_minimum,
                      local_minimum_list<T>& minima_list) {
    if (current_local_minimum == minima_list.end() || (*current_local_minimum).y != Y) {
        return false;
    }
    local_min = &(*current_local_minimum);
    ++current_local_minimum;
    return true;
}
}
}
}
