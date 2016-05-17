#pragma once

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/wagyu/config.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct edge
{
    mapbox::geometry::point<T> Bot;
    mapbox::geometry::point<T> Curr; //current (updated for every new scanbeam)
    mapbox::geometry::point<T> Top;
    double Dx;
    std::size_t OutIdx;
    TEdge *Next;
    TEdge *Prev;
    TEdge *NextInLML;
    TEdge *NextInAEL;
    TEdge *PrevInAEL;
    TEdge *NextInSEL;
    TEdge *PrevInSEL;
    std::int32_t WindCnt;
    std::int32_t WindCnt2; //winding count of the opposite polytype
    std::int8_t WindDelta; //1 or -1 depending on winding direction
    polygon_type PolyTyp;
    edge_type Side; //side only refers to current side of solution poly
};

}}}
