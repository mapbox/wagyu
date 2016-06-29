#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/bound.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct local_minimum {
    bound<T> left_bound;
    bound<T> right_bound;
    T y;
    bool minimum_has_horizontal;

    local_minimum(bound<T>&& left_bound_, bound<T>&& right_bound_, T y_, bool has_horz_)
        : left_bound(left_bound_),
          right_bound(right_bound_),
          y(y_),
          minimum_has_horizontal(has_horz_) {
    }
};

template <typename T>
using local_minimum_list = std::vector<local_minimum<T>>;

template <typename T>
using local_minimum_itr = typename local_minimum_list<T>::iterator;

template <typename T>
using local_minimum_ptr = local_minimum<T>*;

template <typename T>
using local_minimum_ptr_list = std::vector<local_minimum_ptr<T>>;

template <typename T>
using local_minimum_ptr_list_itr = typename local_minimum_ptr_list<T>::iterator;

template <typename T>
struct local_minimum_sorter {
    inline bool operator()(local_minimum_ptr<T> const& locMin1,
                           local_minimum_ptr<T> const& locMin2) {
        if (locMin2->y == locMin1->y) {
            return locMin2->minimum_has_horizontal != locMin1->minimum_has_horizontal &&
                   locMin2->minimum_has_horizontal;
        }
        return locMin2->y < locMin1->y;
    }
};
}
}
}
