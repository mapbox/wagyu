#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct minimum;

template <typename T>
using minimum_ptr = minimum<T> *;

template <typename T>
using const_minimum_ptr = minimum<T> * const;

template <typename T>
struct local_minimum
{
  T            Y;
  edge_ptr<T>  LeftBound;
  edge_ptr<T>  RightBound;
};

template <typename T>
using minimum_list = std::vector<minimum_ptr<T> >;

}}}
