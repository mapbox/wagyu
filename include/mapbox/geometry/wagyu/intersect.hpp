#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct intersect_node {

    sorting_edge_list_itr<T> edge1;
    sorting_edge_list_itr<T> edge2;
    mapbox::geometry::point<T> pt;

    intersect_node(sorting_edge_list_itr<T> const& edge1_,
                   sorting_edge_list_itr<T> const& edge2_,
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
