#pragma once

#include <mapbox/geometry/wagyu/ring.hpp>

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
    std::size_t            Idx;
    intersect_node_ptr<T>  FirstLeft;  //see comments in clipper.pas
    point_ptr<T>           Pts;
    point_ptr<T>           BottomPt;
    bool                   IsHole;
    bool                   IsOpen;
};

template <typename T>
using intersect_list = std::vector<intersect_node_ptr<T> >;

}}}
