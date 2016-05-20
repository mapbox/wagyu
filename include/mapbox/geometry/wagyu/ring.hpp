#pragma once

#include <mapbox/geometry/ring/point.hpp>
#include <mapbox/geometry/ring/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct ring;

template <typename T>
using ring_ptr = ring<T> *;

template <typename T>
using const_ring_ptr = ring<T> * const;

template <typename T>
struct ring
{
    std::size_t   Idx;
    bool          IsHole;
    bool          IsOpen;
    ring_ptr<T>   FirstLeft;  //see comments in clipper.pas
    mapbox::geometry::ring::point_ptr<T>  Pts;
    mapbox::geometry::ring::point_ptr<T>  BottomPt;
};

template <typename T>
using ring_list = std::vector<ring_ptr<T> >;

}}}
