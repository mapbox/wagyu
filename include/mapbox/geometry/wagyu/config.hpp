#pragma once

#include <cassert>
#include <cstdint>
#include <list>
#include <stdexcept>

namespace mapbox {
namespace geometry {
namespace wagyu {

enum clip_type : std::uint8_t {
    clip_type_intersection = 0,
    clip_type_union,
    clip_type_difference,
    clip_type_x_or
};

enum polygon_type : std::uint8_t { polygon_type_subject = 0, polygon_type_clip };

enum fill_type : std::uint8_t {
    fill_type_even_odd = 0,
    fill_type_non_zero,
    fill_type_positive,
    fill_type_negative
};

static double const def_arc_tolerance = 0.25;

static int const EDGE_UNASSIGNED = -1; // edge not currently 'owning' a solution
static int const EDGE_SKIP = -2;       // edge that would otherwise close a path
static std::int64_t const LOW_RANGE = 0x3FFFFFFF;
static std::int64_t const HIGH_RANGE = 0x3FFFFFFFFFFFFFFFLL;

enum horizontal_direction : std::uint8_t { right_to_left = 0, left_to_right = 1 };

enum edge_side : std::uint8_t { edge_left = 0, edge_right };

}
}
}
