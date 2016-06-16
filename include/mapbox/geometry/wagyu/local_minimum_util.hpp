#pragma once

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
