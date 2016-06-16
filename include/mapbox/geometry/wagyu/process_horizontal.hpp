#pragma once

#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
void get_horizontal_direction(edge<T> edge, horizontal_direction& dir, T& left, T& right) {
    if (edge.bot.x < edge.top.x) {
        left = edge.bot.x;
        right = edge.top.x;
        dir = horizontal_direction::left_to_right;
    } else {
        left = edge.top.x;
        right = edge.bot.x;
        dir = horizontal_direction::right_to_left;
    }
}
}
}
}