#pragma once

#include <list>

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/wagyu/config.hpp>

#ifdef DEBUG
#include <iostream>
#endif

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
    using value_type = T;
    mapbox::geometry::point<value_type> Bot;
    mapbox::geometry::point<value_type> Curr; //current (updated for every new scanbeam)
    mapbox::geometry::point<value_type> Top;
    double       Dx;
    int          OutIdx;
    edge_ptr<T>  Next;
    edge_ptr<T>  Prev;
    edge_ptr<T>  NextInLML;
    edge_ptr<T>  NextInAEL;
    edge_ptr<T>  PrevInAEL;
    edge_ptr<T>  NextInSEL;
    edge_ptr<T>  PrevInSEL;
    std::int32_t WindCnt;
    std::int32_t WindCnt2; //winding count of the opposite polytype
    std::int8_t  WindDelta; //1 or -1 depending on winding direction
    polygon_type PolyTyp;
    edge_side    Side; //side only refers to current side of solution poly

    edge(mapbox::geometry::point<value_type> current, 
         mapbox::geometry::point<value_type> next,
         polygon_type type) :
        Bot(current),
        Curr(current),
        Top(current),
        Dx(0.0),
        OutIdx(EDGE_UNASSIGNED),
        Next(nullptr),
        Prev(nullptr),
        NextInLML(nullptr),
        NextInAEL(nullptr),
        PrevInAEL(nullptr),
        NextInSEL(nullptr),
        PrevInSEL(nullptr),
        WindCnt(0),
        WindCnt2(0),
        WindDelta(0),
        PolyTyp(type),
        Side(edge_left)
    {
        if (current.y >= next.y)
        {
            Top = next;
        }
        else
        {
            Bot = next;
        }
        double dy = static_cast<double>(Top.y - Bot.y);
        if (dy == 0.0)
        {
            Dx = HORIZONTAL;
        }
        else
        {
            Dx = static_cast<double>(Top.x - Bot.x) / dy;
        }
    }

};

template <typename T>
using edge_list = std::list<edge<T> >;


#ifdef DEBUG

template <class charT, class traits, typename T>
inline std::basic_ostream<charT,traits>& 
operator << (std::basic_ostream<charT,traits>& out,
             const edge<T>& e)
{
    out << "Edge: " << std::endl;
    out << " Bot x: " << e.Bot.x << " y: " << e.Bot.y << std::endl;
    out << " Top x: " << e.Top.x << " y: " << e.Top.y << std::endl;
    out << " Curr x: " << e.Curr.x << " y: " << e.Curr.y << std::endl;
    return out;
}

#endif                                            

}}}
