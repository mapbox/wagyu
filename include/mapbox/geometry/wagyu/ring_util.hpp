#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
void set_hole_state(edge_ptr<T> e, 
                    ring_ptr<T> ring,
                    ring_list<T> & rings)
{
    edge_ptr<T> e2 = e->prevInAEL;
    edge_ptr<T> eTmp = nullptr;
    while (e2)
    {
        if (e2->index >= 0 && e2->winding_delta != 0)
        {
            if (!eTmp)
            {
                eTmp = e2;
            }
            else if (eTmp->index == e2->index)
            {
                eTmp = nullptr;
            }
        }
        e2 = e2->prevInAEL;
    }

    if (!eTmp)
    {
        ring->first_left = nullptr;
        ring->is_hole = false;
    }
    else
    {
        ring->first_left = rings[eTmp->index];
        ring->is_hole = !ring->first_left->is_hole;
    }
}

template <typename T>
point_ptr<T> add_point(edge_ptr<T> e,
                       mapbox::geometry::point<T> const& pt,
                       ring_list<T> & rings)
{
    if(e->index < 0)
    {
        rings_ptr<T> ring = create_new_ring(rings);
        ring->is_open = (e->winding_delta == 0);
        point_ptr<T> new_point = new point<T>(ring->index, pt);
        ring->points = new_point;
        if (!ring->is_open)
        {
            set_hole_state(e, ring, rings);
        }
        e->index = ring->index;
        return new_point;
    }
    else
    {
        ring_ptr<T> ring = rings[e->index];
        //ring->points is the 'Left-most' point & ring->points->prev is the 'Right-most'
        point_ptr<T> op = ring->points;

        bool ToFront = (e->side == edge_left);
        if (ToFront && (pt == *op))
        {
            return op;
        }
        else if (!ToFront && (pt == *op->prev))
        {
            return op->prev;
        }
        point_ptr<T> new_point = new point<T>(ring->index, pt, op);
        if (ToFront)
        {
            ring->points = new_point;
        }
        return new_point;
    }
}

template <typename T>
point_ptr<T> add_local_minimum_point(edge_ptr<T> e1, 
                                     edge_ptr<T> e2, 
                                     mapbox::geometry::point<T> const& Pt,
                                     ring_list<T> & rings)
{
    point_ptr <T> result;
    edge_ptr<T> e;
    edge_ptr<T> prevE;
    if (is_horizontal(*e2) || ( e1->dx > e2->dx ))
    {
        result = add_point(e1, Pt);
        e2->index = e1->index;
        e1->side = esLeft;
        e2->side = esRight;
        e = e1;
        if (e->prevInAEL == e2)
          prevE = e2->prevInAEL; 
        else
          prevE = e->prevInAEL;
    }
    else
    {
      result = AddOutPt(e2, Pt);
      e1->index = e2->index;
      e1->side = esRight;
      e2->side = esLeft;
      e = e2;
      if (e->prevInAEL == e1)
          prevE = e1->prevInAEL;
      else
          prevE = e->prevInAEL;
    }

    if (prevE && prevE->index >= 0)
    {
      cInt xprev = get_current_x(*prevE, Pt.Y);
      cInt xE = get_current_x(*e, Pt.Y);
      if (xprev == xE && (e->winding_delta != 0) && (prevE->winding_delta != 0) &&
        SlopesEqual(IntPoint(xprev, Pt.Y), prevE->Top, IntPoint(xE, Pt.Y), e->Top, m_UseFullRange))
      {
        OutPt* outPt = AddOutPt(prevE, Pt);
        AddJoin(result, outPt, e->Top);
      }
    }
    return result;
}

}}}
