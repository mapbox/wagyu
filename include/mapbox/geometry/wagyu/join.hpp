#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/wagyu/point.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct join;

template <typename T>
using join_ptr = join<T> *;

template <typename T>
using const_join_ptr = join<T> * const;

template <typename T>
struct join
{
    point_ptr<T>               point1;
    point_ptr<T>               point2;
    mapbox::geometry::point<T> off_point;
};

template <typename T>
using join_list = std::vector<join_ptr<T> >;

}}}
