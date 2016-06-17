#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/wagyu/edge.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {
template <typename T>
struct intersect_node;

template <typename T>
using intersect_node_ptr = intersect_node<T>*;

template <typename T>
using const_intersect_node_ptr = intersect_node<T>* const;

template <typename T>
struct intersect_node {
    edge_ptr<T> edge1;
    edge_ptr<T> edge2;
    mapbox::geometry::point<T> pt;

    intersect_node(const_edge_ptr<T> edge1_,
                   const_edge_ptr<T> edge2_,
                   mapbox::geometry::point<T> pt_)
        : edge1(edge1_), edge2(edge2_), pt(pt_) {
    }
};

template <typename T>
using intersect_list = std::vector<intersect_node<T>>;

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const intersect_node<T>& e) {
    out << "Intersect: " << std::endl;
    out << " point x: " << e.pt.x << " y: " << e.pt.y << std::endl;
    return out;
}

#endif
}
}
}
