#pragma once

#include <mapbox/geometry/point.hpp>

#include <mapbox/geometry/wagyu/sorted_bound_list.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct intersect_node {

    sorting_bound_list_itr<T> bound1;
    sorting_bound_list_itr<T> bound2;
    mapbox::geometry::point<T> pt;

    intersect_node(sorting_bound_list_itr<T> const& bound1_,
                   sorting_bound_list_itr<T> const& bound2_,
                   mapbox::geometry::point<T> pt_)
        : bound1(bound1_), bound2(bound2_), pt(pt_) {
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
