#pragma once

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
    ring_ptr      FirstLeft;  //see comments in clipper.pas
    PolyNode      *PolyNd;
    OutPt    *Pts;
    OutPt    *BottomPt;
};

}}}
