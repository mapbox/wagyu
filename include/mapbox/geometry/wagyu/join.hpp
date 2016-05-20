#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/ring/point.hpp>
#include <mapbox/geometry/ring/ring.hpp>

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
    mapbox::geometry::ring::point_ptr<T>  OutPt1;
    mapbox::geometry::ring::point_ptr<T>  OutPt2;
    mapbox::geometry::point<T>            OffPt;
};

template <typename T>
using join_list = std::vector<join_ptr<T> >;

}}}
