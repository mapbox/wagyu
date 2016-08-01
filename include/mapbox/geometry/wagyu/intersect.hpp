#pragma once

#include <set>

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
    mapbox::geometry::point<double> pt;

    intersect_node(sorting_bound_list_itr<T> const& bound1_,
                   sorting_bound_list_itr<T> const& bound2_,
                   mapbox::geometry::point<double> pt_)
        : bound1(bound1_), bound2(bound2_), pt(pt_) {
    }
};

template <typename T>
using intersect_list = std::vector<intersect_node<T>>;

template <typename T>
struct intersect_point_comparer {
    bool operator() (mapbox::geometry::point<T> const& lhs, mapbox::geometry::point<T> const& rhs) const {
        if (lhs.y == rhs.y) {
            return lhs.x < rhs.x;
        } else {
            return lhs.y < rhs.y;
        }
    }
};

template <typename T>
using hot_pixel_set = std::set<mapbox::geometry::point<T>, intersect_point_comparer<T>>;

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const intersect_node<T>& e) {
    out << " point x: " << e.pt.x << " y: " << e.pt.y << std::endl;
    out << " bound 1: " << std::endl;
    out << *(e.bound1) << std::endl;
    out << " bound 2: " << std::endl;
    out << *(e.bound2) << std::endl;
    return out;
}

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const intersect_list<T>& ints) {
    std::size_t c = 0;
    for (auto const& i : ints) {
        out << "Intersection: " << c++ << std::endl;
        out << i;
    }
    return out;
}

#endif
}
}
}
