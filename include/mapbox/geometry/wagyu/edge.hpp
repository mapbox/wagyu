#pragma once

#include <cmath>
#include <limits>
#include <list>

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/wagyu/config.hpp>

#ifdef DEBUG
#include <iostream>
#endif

namespace mapbox {
namespace geometry {
namespace wagyu {
template <typename T>
struct edge;

template <typename T>
using edge_ptr = edge<T>*;

template <typename T>
using const_edge_ptr = edge<T>* const;

template <typename T>
struct edge {
    using value_type = T;
    mapbox::geometry::point<value_type> bot;
    mapbox::geometry::point<value_type> curr; // updated every new scanbeam
    mapbox::geometry::point<value_type> top;
    double dx;
    ring_ptr<T> ring;
    std::int32_t winding_count;
    std::int32_t winding_count2; // winding count of the opposite polytype
    std::int8_t winding_delta;   // 1 or -1 depending on winding direction
    polygon_type poly_type;
    edge_side side; // side only refers to current side of solution poly

    edge(mapbox::geometry::point<value_type> current,
         mapbox::geometry::point<value_type> next_pt,
         polygon_type type)
        : bot(current),
          curr(current),
          top(current),
          dx(0.0),
          ring(nullptr),
          winding_count(0),
          winding_count2(0),
          winding_delta(0),
          poly_type(type),
          side(edge_left) {
        if (current.y >= next_pt.y) {
            top = next_pt;
        } else {
            bot = next_pt;
        }
        double dy = static_cast<double>(top.y - bot.y);
        if (std::fabs(dy) < std::numeric_limits<double>::epsilon()) {
            dx = std::numeric_limits<double>::infinity();
        } else {
            dx = static_cast<double>(top.x - bot.x) / dy;
        }
    }
};

template <typename T>
using edge_list = std::list<edge<T>>;

template <typename T>
using edge_list_itr = typename edge_list<T>::iterator;

template <typename T>
using edge_list_rev_itr = typename edge_list<T>::reverse_iterator;

template <typename T>
using edge_ptr_list = std::list<edge_ptr<T>>;

template <typename T>
using edge_ptr_list_itr = typename edge_ptr_list<T>::iterator;

template <typename T>
using edge_ptr_list_rev_itr = typename edge_ptr_list<T>::reverse_iterator;

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const edge<T>& e) {
    out << "Edge: " << std::endl;
    out << " bot x: " << e.bot.x << " y: " << e.bot.y << std::endl;
    out << " top x: " << e.top.x << " y: " << e.top.y << std::endl;
    out << " curr x: " << e.curr.x << " y: " << e.curr.y << std::endl;
    return out;
}

#endif
}
}
}
