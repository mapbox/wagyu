#pragma once

#define HORIZONTAL (-1.0E+40)
#define TOLERANCE (1.0e-20)
#define NEAR_ZERO(val) (((val) > -TOLERANCE) && ((val) < TOLERANCE))

namespace mapbox { namespace geometry { namespace wagyu {

enum clip_type : std::uint8_t
{
    clip_type_intersection = 0, 
    clip_type_union, 
    clip_type_difference, 
    clip_type_x_or 
};

enum polygon_type : std::uint8_t
{
    polygon_type_subject = 0, 
    polygon_type_clip
};

enum fill_type : std::uint8_t
{
    fill_type_even_odd = 0, 
    fill_type_non_zero,
    fill_type_positive,
    fill_type_negative
};

static double const def_arc_tolerance = 0.25;

static int const edge_unassigned = -1;  //edge not currently 'owning' a solution
static int const edge_skip = -2;        //edge that would otherwise close a path

enum horizontal_direction : std::uint8_t
{
    right_to_left = 0, 
    left_to_right = 1
};

enum edge_side : std::uint8_t
{
    edge_left = 0, 
    edge_right
};

enum InitOptions : std::uint8_t
{
    ioReverseSolution = 1, 
    ioStrictlySimple = 2, 
    ioPreserveCollinear = 4
};

enum JoinType : std::uint8_t
{
    jtSquare = 0, 
    jtRound, 
    jtMiter
};

enum EndType 
{
    etClosedPolygon = 0, 
    etClosedLine, 
    etOpenButt, 
    etOpenSquare, 
    etOpenRound
};

}}}
