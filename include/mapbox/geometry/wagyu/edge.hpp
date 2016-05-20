#pragma once

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/wagyu/config.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct edge;

template <typename T>
using edge_ptr = edge<T> *;

template <typename T>
using const_edge_ptr = edge<T> * const;

template <typename T>
struct edge
{
    mapbox::geometry::point<T> Bot;
    mapbox::geometry::point<T> Curr; //current (updated for every new scanbeam)
    mapbox::geometry::point<T> Top;
    double       Dx;
    std::size_t  OutIdx;
    edge_ptr     Next;
    edge_ptr     Prev;
    edge_ptr     NextInLML;
    edge_ptr     NextInAEL;
    edge_ptr     PrevInAEL;
    edge_ptr     NextInSEL;
    edge_ptr     PrevInSEL;
    std::int32_t WindCnt;
    std::int32_t WindCnt2; //winding count of the opposite polytype
    std::int8_t  WindDelta; //1 or -1 depending on winding direction
    polygon_type PolyTyp;
    edge_type    Side; //side only refers to current side of solution poly
};

template <typename T>
using edge_list = std::vector<edge_ptr<T> >;

}}}
