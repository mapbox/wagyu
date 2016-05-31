#pragma once

#include <mapbox/geometry/ring/point.hpp>
#include <mapbox/geometry/ring/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct intersect_node;

template <typename T>
using intersect_node_ptr = intersect_node<T> *;

template <typename T>
using const_intersect_node_ptr = intersect_node<T> * const;

template <typename T>
struct intersect_node
{
    std::size_t   Idx;
    bool          IsHole;
    bool          IsOpen;
    intersect_node_ptr<T>   FirstLeft;  //see comments in clipper.pas
    mapbox::geometry::ring::point_ptr<T>  Pts;
    mapbox::geometry::ring::point_ptr<T>  BottomPt;
};

template <typename T>
using intersect_list = std::vector<intersect_node_ptr<T> >;

}}}
