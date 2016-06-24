#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct local_minimum {
    bound<T> left_bound;
    bound<T> right_bound;
    T y;

    local_minimum(bound<T>&& left_bound_, bound<T>&& right_bound_, T y_)
        : left_bound(left_bound_), right_bound(right_bound_), y(y_) {
    }
};

template <typename T>
using local_minimum_list = std::vector<local_minimum<T>>;

template <typename T>
using local_minimum_itr = typename local_minimum_list<T>::iterator;

template <typename T>
using const_local_minimum_ptr = local_minimum<T>* const;

template <typename T>
struct local_minimum_sorter {
    inline bool operator()(const_local_minimum_ptr<T> const& locMin1,
                           const_local_minimum_ptr<T> const& locMin2) {
        return locMin2->y < locMin1->y;
    }
};

template <typename T>
using local_minimum_queue =
    std::priority_queue<const_local_minimum_ptr<T>, std::vector<T>, local_minimum_sorter<T>>;
}
}
}
