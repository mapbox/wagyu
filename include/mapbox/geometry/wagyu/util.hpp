#pragma once

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
double Area(mapbox::geometry::linear_ring<T> const& poly)
{
    std::size_t size = poly.size();
    if (size < 3)
    {
      return 0.0;
    }

    double a = 0.0;
    auto itr = poly.begin();
    auto itr_prev = poly.end();
    --itr_prev;
    a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    ++itr;
    itr_prev = poly.begin();
    for (;itr != poly.end(); ++itr, ++itr_next)
    {
        a += static_cast<double>(itr_prev->x + itr->x) * static_cast<double>(itr_prev->y - itr->y);
    }
    return -a * 0.5;
}

template <typename T>
double Area(point_ptr<T> op)
{
    point_ptr<T> startOp = op;
    if (!op)
    {
        return 0.0;
    }
    double a = 0.0;
    do 
    {
        a += static_cast<double>(op->prev->x + op->x) * static_cast<double>(op->prev->y - op->y);
        op = op->next;
    }
    while (op != startOp);
    return a * 0.5;
}

template <typename T>
double Area(ring<T> const& polygon_ring)
{
  return Area(polygon_ring.Pts);
}

template <typename T>
bool Orientation(mapbox::geometry::linear_ring<T> const& poly)
{
    return Area(poly) >= 0;
}

template <typename T>
bool PointIsVertex(mapbox::geometry::point<T> const& Pt, point_ptr<T> pp)
{
    point_ptr<T> pp2 = pp;
    do
    {
        if (*pp2 == Pt)
        {
            return true;
        }
        pp2 = pp2->next;
    }
    while (pp2 != pp);
    return false;
}

enum point_in_polygon_result : std::int8_t
{
    point_on_polygon = -1,
    point_inside_polygon = 0,
    point_outside_polygon = 1
}

template <typename T>
point_in_polygon_result PointInPolygon(point<T> const& pt, mapbox::geometry::linear_ring<T> const& path)
{
    //returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    std::size_t cnt = path.size();
    if (cnt < 3)
    {
        return point_outside_polygon;
    }
    point_in_polygon_result result = point_outside_polygon;
    auto itr = path.begin();
    auto itr_prev = path.end();
    --itr_prev;
    if (itr->y == pt.y)
    {
        if ((itr->x == pt.x) || (itr_prev->y == pt.y && ((itr->x > pt.x) == (itr_prev->x < pt.x))))
        {
            return point_on_polygon;
        }
    }
    if ((itr_prev->y < pt.y) != (itr->y < pt.y))
    {
        if (itr_prev->x >= pt.x)
        {
            if (itr->x > pt.x)
            {
                // Switch between point outside polygon and point inside polygon
                result = 1 - result;
            }
            else
            {
                double d = static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) - 
                        static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                if (!d)
                {
                    return point_on_polygon;
                }
                if ((d > 0) == (itr->y > itr_prev->y))
                {
                    // Switch between point outside polygon and point inside polygon
                    result = 1 - result;
                }
            }
        } 
        else
        {
            if (itr->x > pt.x)
            {
                double d = static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) - 
                        static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                if (!d)
                {
                    return point_on_polygon;
                }
                if ((d > 0) == (itr->y > itr_prev->y))
                {
                    // Switch between point outside polygon and point inside polygon
                    result = 1 - result;
                }
            }
        }
    }
    ++itr;
    itr_prev = poly.begin();
    for (; itr != path.end(); ++itr, ++itr_prev)
    {
        if (itr->y == pt.y)
        {
            if ((itr->x == pt.x) || (itr_prev->y == pt.y && ((itr->x > pt.x) == (itr_prev->x < pt.x))))
            {
                return point_on_polygon;
            }
        }
        if ((itr_prev->y < pt.y) != (itr->y < pt.y))
        {
            if (itr_prev->x >= pt.x)
            {
                if (itr->x > pt.x)
                {
                    // Switch between point outside polygon and point inside polygon
                    result = 1 - result;
                }
                else
                {
                    double d = static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) - 
                            static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                    if (!d)
                    {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (itr->y > itr_prev->y))
                    {
                        // Switch between point outside polygon and point inside polygon
                        result = 1 - result;
                    }
                }
            } 
            else
            {
                if (itr->x > pt.x)
                {
                    double d = static_cast<double>(itr_prev->x - pt.x) * static_cast<double>(itr->y - pt.y) - 
                            static_cast<double>(itr->x - pt.x) * static_cast<double>(itr_prev->y - pt.y);
                    if (!d)
                    {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (itr->y > itr_prev->y))
                    {
                        // Switch between point outside polygon and point inside polygon
                        result = 1 - result;
                    }
                }
            }
        }
    } 
    return result;
}

template <typename T>
point_in_polygon_result PointInPolygon(point<T> const& pt, point_ptr<T> op)
{
    //returns 0 if false, +1 if true, -1 if pt ON polygon boundary
    point_in_polygon_result result = point_outside_polygon;
    point_ptr<T> startOp = op;
    do
    {
        if (op->next->y == pt.y)
        {
            if ((op->next->x == pt.x) || (op->y == pt.y && ((op->next->x > pt.x) == (op->x < pt.x))))
            {
                return point_on_polygon;
            }
        }
        if ((op->y < pt.y) != (op->next->y < pt.y))
        {
            if (op->x >= pt.x)
            {
                if (op->next->x > pt.x)
                {
                    // Switch between point outside polygon and point inside polygon
                    result = 1 - result;
                }
                else
                {
                    double d = static_cast<double>(op->x - pt.x) * static_cast<double>(op->next->y - pt.y) - 
                            static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (!d)
                    {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->y > op->y))
                    {
                        // Switch between point outside polygon and point inside polygon
                        result = 1 - result;
                    }
                }
            }
            else
            {
                if (op->next->Pt.X > pt.X)
                {
                    double d = static_cast<double>(op->x - pt.x) * static_cast<double>(op->next->y - pt.y) - 
                            static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (!d)
                    {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->Pt.Y > op->Pt.Y))
                    {
                        // Switch between point outside polygon and point inside polygon
                        result = 1 - result;
                    }
                }
            }
        } 
        op = op->next;
    }
    while (startOp != op);
    return result;
}

template <typename T>
bool Poly2ContainsPoly1(point_ptr<T> OutPt1, point_ptr<T> OutPt2)
{
    point_ptr<T> op = OutPt1;
    do
    {
        //nb: PointInPolygon returns 0 if false, +1 if true, -1 if pt on polygon
        point_in_polygon_result res = PointInPolygon(*op, OutPt2);
        if (res != point_on_polygon)
        {
            if (res == point_inside_polygon)
            {
                return true;
            }
            else
            {
                return false;
            }
        }
        op = op->Next; 
    }
    while (op != OutPt1);
    return true; 
}

template <typename T>
bool SlopesEqual(edge<T> const& e1, edge<T> const& e2)
{
    return (e1.Top.y - e1.Bot.y) * (e2.Top.x - e2.Bot.x) == (e1.Top.x - e1.Bot.x) * (e2.Top.y - e2.Bot.y);
}

template <typename T>
bool SlopesEqual(point<T> const& pt1, point<T> const& pt2, point<T> const& pt3)
{
    return (pt1.y - pt2.y) * (pt2.x - pt3.x) == (pt1.x - pt2.x) * (pt2.y - pt3.y);
}

template <typename T>
bool SlopesEqual(point<T> const& pt1, point<T> const& pt2, point<T> const& pt3, point<T> const& pt4)
{
    return (pt1.y - pt2.y) * (pt3.x - pt4.x) == (pt1.x - pt2.x) * (pt3.y - pt4.y);
}

template <typename T>
inline bool IsHorizontal(edge<T> const& e)
{
    return e.Dx == HORIZONTAL;
}

template <typename T>
inline double GetDx(point<T> const& pt1, point<T> const& pt2)
{
    if (pt1.y == pt2.y)
    {
        return HORIZONTAL;
    }
    else
    {
        return static_cast<double>(pt2.x - pt2.x) / static_cast<double>(pt2.y - pt1.y);
    }
}

template <typename T>
inline void SetDx(edge<T> & e)
{
    double dy = static_cast<double>(e.Top.y - e.Bot.y);
    if (dy == 0.0)
    {
        e.Dx = HORIZONTAL;
    }
    else
    {
        e.Dx = static_cast<double>(e.Top.x - e.Bot.x) / dy;
    }
}

template <typename T>
inline void SwapSides(edge<T> & Edge1, edge<T> & Edge2)
{
    edge_side Side =  Edge1.Side;
    Edge1.Side = Edge2.Side;
    Edge2.Side = Side;
}

template <typename T>
inline void SwapPolyIndexes(edge<T> & Edge1, edge<T> & Edge2)
{
  std::size_t OutIdx =  Edge1.OutIdx;
  Edge1.OutIdx = Edge2.OutIdx;
  Edge2.OutIdx = OutIdx;
}

inline cInt TopX(TEdge &edge, const cInt currentY)
{
  return ( currentY == edge.Top.Y ) ?
    edge.Top.X : edge.Bot.X + Round(edge.Dx *(currentY - edge.Bot.Y));
}
//------------------------------------------------------------------------------

void IntersectPoint(TEdge &Edge1, TEdge &Edge2, IntPoint &ip)
{
#ifdef use_xyz  
  ip.Z = 0;
#endif

  double b1, b2;
  if (Edge1.Dx == Edge2.Dx)
  {
    ip.Y = Edge1.Curr.Y;
    ip.X = TopX(Edge1, ip.Y);
    return;
  }
  else if (Edge1.Dx == 0.0)
  {
    ip.X = Edge1.Bot.X;
    if (IsHorizontal(Edge2))
      ip.Y = Edge2.Bot.Y;
    else
    {
      b2 = Edge2.Bot.Y - (Edge2.Bot.X / Edge2.Dx);
      if (Edge2.Bot.X == Edge1.Bot.X) ip.Y = Round(ip.X / Edge2.Dx + b2);
      else if (Edge2.Bot.X < Edge1.Bot.X) ip.Y = Round((ip.X - 0.5) / Edge2.Dx + b2);
      else ip.Y = Round((ip.X + 0.5) / Edge2.Dx + b2);
    }
  }
  else if (Edge2.Dx == 0.0)
  {
    ip.X = Edge2.Bot.X;
    if (IsHorizontal(Edge1))
      ip.Y = Edge1.Bot.Y;
    else
    {
      b1 = Edge1.Bot.Y - (Edge1.Bot.X / Edge1.Dx);
      if (Edge1.Bot.X == Edge2.Bot.X) ip.Y = Round(ip.X / Edge1.Dx + b1);
      else if (Edge1.Bot.X < Edge2.Bot.X) ip.Y = Round((ip.X - 0.5) / Edge1.Dx + b1);
      else ip.Y = Round((ip.X + 0.5) / Edge1.Dx + b1);
    }
  } 
  else 
  {
    b1 = Edge1.Bot.X - Edge1.Bot.Y * Edge1.Dx;
    b2 = Edge2.Bot.X - Edge2.Bot.Y * Edge2.Dx;
    double q = (b2-b1) / (Edge1.Dx - Edge2.Dx);
    ip.Y = Round(q);
    if (std::fabs(Edge1.Dx) < std::fabs(Edge2.Dx))
      ip.X = Round(Edge1.Dx * q + b1);
    else 
      ip.X = Round(Edge2.Dx * q + b2);
    // the idea is simply to looking closer
    // towards the origins of the lines (Edge1.Bot and Edge2.Bot)
    // until we do not find pixels that both lines travel through
    bool keep_searching = false;
    double by1 = Edge1.Bot.Y - (Edge1.Bot.X / Edge1.Dx);
    double by2 = Edge2.Bot.Y - (Edge2.Bot.X / Edge2.Dx);
    double bx1 = Edge1.Bot.X - (Edge1.Bot.Y * Edge1.Dx);
    double bx2 = Edge2.Bot.X - (Edge2.Bot.Y * Edge2.Dx);
    do
    {
        keep_searching = false;
        cInt y1 = ip.Y;
        cInt y2 = ip.Y;
        if (Edge1.Bot.X > ip.X)
        {
            if (Edge1.Bot.Y >= ip.Y)
            {
                y1 = std::floor(((ip.X + 0.5) / Edge1.Dx + by1) + 0.5);
            }
            else
            {
                y1 = std::ceil(((ip.X + 0.5) / Edge1.Dx + by1) - 0.5);
            }
        }
        else if (Edge1.Bot.X < ip.X)
        {
            if (Edge1.Bot.Y >= ip.Y)
            {
                y1 = std::floor(((ip.X - 0.5) / Edge1.Dx + by1) + 0.5);
            }
            else
            {
                y1 = std::ceil(((ip.X - 0.5) / Edge1.Dx + by1) - 0.5);
            }
        }
        else if (Edge1.Bot.Y > ip.Y)
        {
            if (Edge2.Bot.Y >= Edge1.Bot.Y)
            {
                y1 = Edge1.Bot.Y;
            }
            else
            {
                y1 = Edge2.Bot.Y;
            }
        }
        else if (Edge1.Bot.Y < ip.Y)
        {
            if (Edge2.Bot.Y <= Edge1.Bot.Y)
            {
                y1 = Edge1.Bot.Y;
            }
            else
            {
                y1 = Edge2.Bot.Y;
            }
        }
        if (ip.Y >= Edge1.Bot.Y && y1 < Edge1.Bot.Y) y1 = Edge1.Bot.Y;
        else if (ip.Y <= Edge1.Bot.Y && y1 > Edge1.Bot.Y) y1 = Edge1.Bot.Y;
        if (Edge2.Bot.X > ip.X)
        {
            if (Edge2.Bot.Y >= ip.Y)
            {
                y2 = std::floor(((ip.X + 0.5) / Edge2.Dx + by2) + 0.5);
            }
            else
            {
                y2 = std::ceil(((ip.X + 0.5) / Edge2.Dx + by2) - 0.5);
            }
        }
        else if (Edge2.Bot.X < ip.X)
        {
            if (Edge2.Bot.Y >= ip.Y)
            {
                y2 = std::floor(((ip.X - 0.5) / Edge2.Dx + by2) + 0.5);
            }
            else
            {
                y2 = std::ceil(((ip.X - 0.5) / Edge2.Dx + by2) - 0.5);
            }
        }
        else if (Edge2.Bot.Y > ip.Y)
        {
            if (Edge1.Bot.Y >= Edge2.Bot.Y)
            {
                y2 = Edge2.Bot.Y;
            }
            else
            {
                y2 = Edge1.Bot.Y;
            }
        }
        else if (Edge2.Bot.Y < ip.Y)
        {
            if (Edge1.Bot.Y <= Edge2.Bot.Y)
            {
                y2 = Edge2.Bot.Y;
            }
            else
            {
                y2 = Edge1.Bot.Y;
            }
        }
        if (ip.Y >= Edge2.Bot.Y && y2 < Edge2.Bot.Y) y2 = Edge2.Bot.Y;
        else if (ip.Y <= Edge2.Bot.Y && y2 > Edge2.Bot.Y) y2 = Edge2.Bot.Y;
        cInt x1 = ip.X;
        cInt x2 = ip.X;
        if (Edge1.Bot.Y > ip.Y)
        {
            if (Edge1.Bot.X >= ip.X)
            {
                x1 = std::floor(((ip.Y + 0.5) * Edge1.Dx + bx1) + 0.5);
            }
            else
            {
                x1 = std::ceil(((ip.Y + 0.5) * Edge1.Dx + bx1) - 0.5);
            }
        }
        else if (Edge1.Bot.Y < ip.Y)
        {
            if (Edge1.Bot.X >= ip.X)
            {
                x1 = std::floor(((ip.Y - 0.5) * Edge1.Dx + bx1) + 0.5);
            }
            else
            {
                x1 = std::ceil(((ip.Y - 0.5) * Edge1.Dx + bx1) - 0.5);
            }
        }
        else if (Edge1.Bot.X > ip.X)
        {
            if (Edge2.Bot.X >= Edge1.Bot.X)
            {
                x1 = Edge1.Bot.X;
            }
            else
            {
                x1 = Edge2.Bot.X;
            }
        }
        else if (Edge1.Bot.X < ip.X)
        {
            if (Edge2.Bot.X <= Edge1.Bot.X)
            {
                x1 = Edge1.Bot.X;
            }
            else
            {
                x1 = Edge2.Bot.X;
            }
        }
        if (ip.X >= Edge1.Bot.X && x1 < Edge1.Bot.X) x1 = Edge1.Bot.X;
        else if (ip.X <= Edge1.Bot.X && x1 > Edge1.Bot.X) x1 = Edge1.Bot.X;
        if (Edge2.Bot.Y > ip.Y)
        {
            if (Edge2.Bot.X >= ip.X)
            {
                x2 = std::floor(((ip.Y + 0.5) * Edge2.Dx + bx2) + 0.5);
            }
            else
            {
                x2 = std::ceil(((ip.Y + 0.5) * Edge2.Dx + bx2) - 0.5);
            }
        }
        else if (Edge2.Bot.Y < ip.Y)
        {
            if (Edge2.Bot.X >= ip.X)
            {
                x2 = std::floor(((ip.Y - 0.5) * Edge2.Dx + bx2) + 0.5);
            }
            else
            {
                x2 = std::ceil(((ip.Y - 0.5) * Edge2.Dx + bx2) - 0.5);
            }
        }
        else if (Edge2.Bot.X > ip.X)
        {
            if (Edge1.Bot.X >= Edge2.Bot.X)
            {
                x2 = Edge2.Bot.X;
            }
            else
            {
                x2 = Edge1.Bot.X;
            }
        }
        else if (Edge2.Bot.X < ip.X)
        {
            if (Edge1.Bot.X <= Edge2.Bot.X)
            {
                x2 = Edge2.Bot.X;
            }
            else
            {
                x2 = Edge1.Bot.X;
            }
        }
        if (ip.X >= Edge2.Bot.X && x2 < Edge2.Bot.X) x2 = Edge2.Bot.X;
        else if (ip.X <= Edge2.Bot.X && x2 > Edge2.Bot.X) x2 = Edge2.Bot.X;
        if (y1 > ip.Y && y2 > ip.Y)
        {
            ip.Y = std::min(y1,y2);
            keep_searching = true;
        }
        else if (y1 < ip.Y && y2 < ip.Y)
        {
            ip.Y = std::max(y1,y2);
            keep_searching = true;
        } 
        if (x1 > ip.X && x2 > ip.X)
        {
            ip.X = std::min(x1,x2);
            keep_searching = true;
        }
        else if (x1 < ip.X && x2 < ip.X)
        {
            ip.X = std::max(x1,x2);
            keep_searching = true;
        }
    }
    while (keep_searching);
  }

  if (ip.Y < Edge1.Top.Y || ip.Y < Edge2.Top.Y) 
  {
    if (Edge1.Top.Y > Edge2.Top.Y)
      ip.Y = Edge1.Top.Y;
    else
      ip.Y = Edge2.Top.Y;
    if (std::fabs(Edge1.Dx) < std::fabs(Edge2.Dx))
      ip.X = TopX(Edge1, ip.Y);
    else
      ip.X = TopX(Edge2, ip.Y);
  } 
  //finally, don't allow 'ip' to be BELOW curr.Y (ie bottom of scanbeam) ...
  if (ip.Y > Edge1.Curr.Y)
  {
    ip.Y = Edge1.Curr.Y;
    //use the more vertical edge to derive X ...
    if (std::fabs(Edge1.Dx) > std::fabs(Edge2.Dx))
      ip.X = TopX(Edge2, ip.Y); else
      ip.X = TopX(Edge1, ip.Y);
  }
}
//------------------------------------------------------------------------------

void ReversePolyPtLinks(OutPt *pp)
{
  if (!pp) return;
  OutPt *pp1, *pp2;
  pp1 = pp;
  do {
  pp2 = pp1->Next;
  pp1->Next = pp1->Prev;
  pp1->Prev = pp2;
  pp1 = pp2;
  } while( pp1 != pp );
}
//------------------------------------------------------------------------------

void DisposeOutPts(OutPt*& pp)
{
  if (pp == 0) return;
    pp->Prev->Next = 0;
  while( pp )
  {
    OutPt *tmpPp = pp;
    pp = pp->Next;
    delete tmpPp;
  }
}
//------------------------------------------------------------------------------

inline void InitEdge(TEdge* e, TEdge* eNext, TEdge* ePrev, const IntPoint& Pt)
{
  std::memset(e, 0, sizeof(TEdge));
  e->Next = eNext;
  e->Prev = ePrev;
  e->Curr = Pt;
  e->OutIdx = Unassigned;
}
//------------------------------------------------------------------------------

void InitEdge2(TEdge& e, PolyType Pt)
{
  if (e.Curr.Y >= e.Next->Curr.Y)
  {
    e.Bot = e.Curr;
    e.Top = e.Next->Curr;
  } else
  {
    e.Top = e.Curr;
    e.Bot = e.Next->Curr;
  }
  SetDx(e);
  e.PolyTyp = Pt;
}
//------------------------------------------------------------------------------

TEdge* RemoveEdge(TEdge* e)
{
  //removes e from double_linked_list (but without removing from memory)
  e->Prev->Next = e->Next;
  e->Next->Prev = e->Prev;
  TEdge* result = e->Next;
  e->Prev = 0; //flag as removed (see ClipperBase.Clear)
  return result;
}
//------------------------------------------------------------------------------

inline void ReverseHorizontal(TEdge &e)
{
  //swap horizontal edges' Top and Bottom x's so they follow the natural
  //progression of the bounds - ie so their xbots will align with the
  //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
  std::swap(e.Top.X, e.Bot.X);
#ifdef use_xyz  
  std::swap(e.Top.Z, e.Bot.Z);
#endif
}
//------------------------------------------------------------------------------

void SwapPoints(IntPoint &pt1, IntPoint &pt2)
{
  IntPoint tmp = pt1;
  pt1 = pt2;
  pt2 = tmp;
}
//------------------------------------------------------------------------------

bool GetOverlapSegment(IntPoint pt1a, IntPoint pt1b, IntPoint pt2a,
  IntPoint pt2b, IntPoint &pt1, IntPoint &pt2)
{
  //precondition: segments are Collinear.
  if (Abs(pt1a.X - pt1b.X) > Abs(pt1a.Y - pt1b.Y))
  {
    if (pt1a.X > pt1b.X) SwapPoints(pt1a, pt1b);
    if (pt2a.X > pt2b.X) SwapPoints(pt2a, pt2b);
    if (pt1a.X > pt2a.X) pt1 = pt1a; else pt1 = pt2a;
    if (pt1b.X < pt2b.X) pt2 = pt1b; else pt2 = pt2b;
    return pt1.X < pt2.X;
  } else
  {
    if (pt1a.Y < pt1b.Y) SwapPoints(pt1a, pt1b);
    if (pt2a.Y < pt2b.Y) SwapPoints(pt2a, pt2b);
    if (pt1a.Y < pt2a.Y) pt1 = pt1a; else pt1 = pt2a;
    if (pt1b.Y > pt2b.Y) pt2 = pt1b; else pt2 = pt2b;
    return pt1.Y > pt2.Y;
  }
}
//------------------------------------------------------------------------------

bool FirstIsBottomPt(const OutPt* btmPt1, const OutPt* btmPt2)
{
  OutPt *p = btmPt1->Prev;
  while ((p->Pt == btmPt1->Pt) && (p != btmPt1)) p = p->Prev;
  double dx1p = std::fabs(GetDx(btmPt1->Pt, p->Pt));
  p = btmPt1->Next;
  while ((p->Pt == btmPt1->Pt) && (p != btmPt1)) p = p->Next;
  double dx1n = std::fabs(GetDx(btmPt1->Pt, p->Pt));

  p = btmPt2->Prev;
  while ((p->Pt == btmPt2->Pt) && (p != btmPt2)) p = p->Prev;
  double dx2p = std::fabs(GetDx(btmPt2->Pt, p->Pt));
  p = btmPt2->Next;
  while ((p->Pt == btmPt2->Pt) && (p != btmPt2)) p = p->Next;
  double dx2n = std::fabs(GetDx(btmPt2->Pt, p->Pt));

  if (std::max(dx1p, dx1n) == std::max(dx2p, dx2n) &&
    std::min(dx1p, dx1n) == std::min(dx2p, dx2n))
      return Area(btmPt1) > 0; //if otherwise identical use orientation
  else
    return (dx1p >= dx2p && dx1p >= dx2n) || (dx1n >= dx2p && dx1n >= dx2n);
}
//------------------------------------------------------------------------------

OutPt* GetBottomPt(OutPt *pp)
{
  OutPt* dups = 0;
  OutPt* p = pp->Next;
  while (p != pp)
  {
    if (p->Pt.Y > pp->Pt.Y)
    {
      pp = p;
      dups = 0;
    }
    else if (p->Pt.Y == pp->Pt.Y && p->Pt.X <= pp->Pt.X)
    {
      if (p->Pt.X < pp->Pt.X)
      {
        dups = 0;
        pp = p;
      } else
      {
        if (p->Next != pp && p->Prev != pp) dups = p;
      }
    }
    p = p->Next;
  }
  if (dups)
  {
    //there appears to be at least 2 vertices at BottomPt so ...
    while (dups != p)
    {
      if (!FirstIsBottomPt(p, dups)) pp = dups;
      dups = dups->Next;
      while (dups->Pt != pp->Pt) dups = dups->Next;
    }
  }
  return pp;
}
//------------------------------------------------------------------------------

bool Pt2IsBetweenPt1AndPt3(const IntPoint pt1,
  const IntPoint pt2, const IntPoint pt3)
{
  if ((pt1 == pt3) || (pt1 == pt2) || (pt3 == pt2))
    return false;
  else if (pt1.X != pt3.X)
    return (pt2.X > pt1.X) == (pt2.X < pt3.X);
  else
    return (pt2.Y > pt1.Y) == (pt2.Y < pt3.Y);
}
//------------------------------------------------------------------------------

bool HorzSegmentsOverlap(cInt seg1a, cInt seg1b, cInt seg2a, cInt seg2b)
{
  if (seg1a > seg1b) std::swap(seg1a, seg1b);
  if (seg2a > seg2b) std::swap(seg2a, seg2b);
  return (seg1a < seg2b) && (seg2a < seg1b);
}

}}}
