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
struct bound;

template <typename T>
using bound_ptr = bound<T>*;

template <typename T>
struct edge {
    mapbox::geometry::point<T> bot;
    mapbox::geometry::point<T> top;
    double dx;
    bound_ptr<T> bound; // the bound to which an edge belongs

    edge(mapbox::geometry::point<T> current, mapbox::geometry::point<T> next_pt)
        : bot(current), top(current), dx(0.0) {
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
using edge_ptr = edge<T>*;

template <typename T>
using const_edge_ptr = edge<T>* const;

template <typename T>
using edge_list = std::list<edge<T>>;

template <typename T>
using edge_list_itr = typename edge_list<T>::iterator;

#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const edge<T>& e) {
    out << "    Edge: " << std::endl;
    out << "        bot x: " << e.bot.x << " y: " << e.bot.y << std::endl;
    out << "        top x: " << e.top.x << " y: " << e.top.y << std::endl;
    return out;
}

template <class charT, class traits, typename T>
inline std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& out,
                                                     const edge_list<T>& edges) {
    std::size_t c = 0;
    for (auto const& e : edges) {
        out << "Index: " << c++ << std::endl;
        out << e;
    }
    return out;
}

#endif
}
}
}
