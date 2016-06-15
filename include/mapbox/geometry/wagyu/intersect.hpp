#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/wagyu/edge.hpp>

namespace mapbox
{
namespace geometry
{
namespace wagyu
{
template <typename T>
struct intersect_node;

template <typename T>
using intersect_node_ptr = intersect_node<T> *;

template <typename T>
using const_intersect_node_ptr = intersect_node<T> * const;

template <typename T>
struct intersect_node
{
    edge_ptr<T> edge1;
    edge_ptr<T> edge2;
    mapbox::geometry::point<T> pt;

    intersect_node(const_edge_ptr<T> edge1_,
                   const_edge_ptr<T> edge2_,
                   mapbox::geometry::point<T> pt_)
        : edge1(edge1_), edge2(edge2_), pt(pt_)
    {
    }
};

template <typename T>
using intersect_list = std::vector<intersect_node_ptr<T> >;
}
}
}
