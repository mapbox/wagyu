#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct local_minimum;

template <typename T>
using local_minimum_ptr = local_minimum<T> *;

template <typename T>
using const_local_minimum_ptr = local_minimum<T> * const;

template <typename T>
struct local_minimum
{
    T            Y;
    edge_ptr<T>  LeftBound;
    edge_ptr<T>  RightBound;
};

template <typename T>
using local_minimum_list = std::vector<local_minimum<T> >;

template <typename T>
struct local_minimum_sorter
{
    inline bool operator()(local_minimum<T> const& locMin1, local_minimum<T> const& locMin2)
    {
        return locMin2.Y < locMin1.Y;
    }
};

}}}
