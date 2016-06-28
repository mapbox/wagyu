#pragma once

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
inline void swap_rings(edge<T>& e1, edge<T>& e2) {
    ring_ptr<T> ring = e1.ring;
    e1.ring = e2.ring;
    e2.ring = ring;
}

template <typename T>
inline void swap_sides(edge<T>& e1, edge<T>& e2) {
    edge_side side = e1.side;
    e1.side = e2.side;
    e2.side = side;
}

template <typename T>
inline void reverse_horizontal(edge<T>& e) {
    // swap horizontal edges' top and bottom x's so they follow the natural
    // progression of the bounds - ie so their xbots will align with the
    // adjoining lower edge. [Helpful in the process_horizontal() method.]
    std::swap(e.top.x, e.bot.x);
}
}
}
}
