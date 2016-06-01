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
    for (;itr != poly.end(); ++itr, ++itr_prev)
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
};

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
                if (result == point_outside_polygon)
                {
                    result = point_inside_polygon;
                }
                else
                {
                    result = point_outside_polygon;
                }
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
                    if (result == point_outside_polygon)
                    {
                        result = point_inside_polygon;
                    }
                    else
                    {
                        result = point_outside_polygon;
                    }
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
                    if (result == point_outside_polygon)
                    {
                        result = point_inside_polygon;
                    }
                    else
                    {
                        result = point_outside_polygon;
                    }
                }
            }
        }
    }
    ++itr;
    itr_prev = path.begin();
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
                    if (result == point_outside_polygon)
                    {
                        result = point_inside_polygon;
                    }
                    else
                    {
                        result = point_outside_polygon;
                    }
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
                        if (result == point_outside_polygon)
                        {
                            result = point_inside_polygon;
                        }
                        else
                        {
                            result = point_outside_polygon;
                        }
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
                        if (result == point_outside_polygon)
                        {
                            result = point_inside_polygon;
                        }
                        else
                        {
                            result = point_outside_polygon;
                        }
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
                    if (result == point_outside_polygon)
                    {
                        result = point_inside_polygon;
                    }
                    else
                    {
                        result = point_outside_polygon;
                    }
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
                        if (result == point_outside_polygon)
                        {
                            result = point_inside_polygon;
                        }
                        else
                        {
                            result = point_outside_polygon;
                        }
                    }
                }
            }
            else
            {
                if (op->next->Pt.x > pt.x)
                {
                    double d = static_cast<double>(op->x - pt.x) * static_cast<double>(op->next->y - pt.y) - 
                            static_cast<double>(op->next->x - pt.x) * static_cast<double>(op->y - pt.y);
                    if (!d)
                    {
                        return point_on_polygon;
                    }
                    if ((d > 0) == (op->next->Pt.y > op->Pt.y))
                    {
                        // Switch between point outside polygon and point inside polygon
                        if (result == point_outside_polygon)
                        {
                            result = point_inside_polygon;
                        }
                        else
                        {
                            result = point_outside_polygon;
                        }
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
bool SlopesEqual(edge<T> const& e1,
                 edge<T> const& e2)
{
    return (e1.Top.y - e1.Bot.y) * (e2.Top.x - e2.Bot.x) == (e1.Top.x - e1.Bot.x) * (e2.Top.y - e2.Bot.y);
}

template <typename T>
bool SlopesEqual(mapbox::geometry::point<T> const& pt1, 
                 mapbox::geometry::point<T> const& pt2, 
                 mapbox::geometry::point<T> const& pt3)
{
    return (pt1.y - pt2.y) * (pt2.x - pt3.x) == (pt1.x - pt2.x) * (pt2.y - pt3.y);
}

template <typename T>
bool SlopesEqual(mapbox::geometry::point<T> const& pt1,
                 mapbox::geometry::point<T> const& pt2, 
                 mapbox::geometry::point<T> const& pt3, 
                 mapbox::geometry::point<T> const& pt4)
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

template <typename T>
inline T TopX(edge<T> const& edge, const T currentY)
{
    if (currentY == edge.Top.y)
    {
        return edge.Top.x;
    }
    else
    {
        return edge.Bot.x + static_cast<T>(std::round(edge.Dx * static_cast<double>(currentY - edge.Bot.y)));
    }
}

template <typename T>
void ReversePolyPtLinks(point_ptr<T> pp)
{
    if (!pp)
    {
        return;
    }
    point_ptr<T> pp1;
    point_ptr<T> pp2;
    pp1 = pp;
    do
    {
        pp2 = pp1->Next;
        pp1->Next = pp1->Prev;
        pp1->Prev = pp2;
        pp1 = pp2;
    } 
    while( pp1 != pp );
}

template <typename T>
void DisposeOutPts(point_ptr<T> & pp)
{
    if (pp == nullptr)
    {
      return;
    }
    pp->Prev->Next = nullptr;
    while (pp)
    {
      point_ptr<T> tmpPp = pp;
      pp = pp->Next;
      delete tmpPp;
    }
}

template <typename T>
void SwapPoints(mapbox::geometry::point<T> &pt1, mapbox::geometry::point<T> &pt2)
{
    mapbox::geometry::point<T> tmp = pt1;
    pt1 = pt2;
    pt2 = tmp;
}

template <typename T>
bool GetOverlapSegment(mapbox::geometry::point<T> pt1a, 
                       mapbox::geometry::point<T> pt1b,
                       mapbox::geometry::point<T> pt2a,
                       mapbox::geometry::point<T> pt2b, 
                       mapbox::geometry::point<T> &pt1, 
                       mapbox::geometry::point<T> &pt2)
{
    //precondition: segments are Collinear.
    if (Abs(pt1a.x - pt1b.x) > Abs(pt1a.y - pt1b.y))
    {
        if (pt1a.x > pt1b.x)
        {
            SwapPoints(pt1a, pt1b);
        }
        if (pt2a.x > pt2b.x)
        {
            SwapPoints(pt2a, pt2b);
        }
        if (pt1a.x > pt2a.x)
        {
            pt1 = pt1a;
        }
        else 
        {
            pt1 = pt2a;
        }
        if (pt1b.x < pt2b.x)
        {
            pt2 = pt1b;
        }
        else 
        {
            pt2 = pt2b;
        }
        return pt1.x < pt2.x;
    }
    else
    {
        if (pt1a.y < pt1b.y)
        {
            SwapPoints(pt1a, pt1b);
        }
        if (pt2a.y < pt2b.y)
        {
            SwapPoints(pt2a, pt2b);
        }
        if (pt1a.y < pt2a.y)
        {
            pt1 = pt1a;
        }
        else
        {
            pt1 = pt2a;
        }
        if (pt1b.y > pt2b.y)
        {
            pt2 = pt1b;
        }
        else
        {
            pt2 = pt2b;
        }
        return pt1.y > pt2.y;
    }
}

template <typename T>
bool FirstIsBottomPt(const_point_ptr<T> btmPt1, const_point_ptr<T> btmPt2)
{
    point_ptr<T> p = btmPt1->Prev;
    while ((p->Pt == btmPt1->Pt) && (p != btmPt1))
    {
        p = p->Prev;
    }
    double dx1p = std::fabs(GetDx(btmPt1->Pt, p->Pt));
    p = btmPt1->Next;
    while ((p->Pt == btmPt1->Pt) && (p != btmPt1))
    {
        p = p->Next;
    }
    double dx1n = std::fabs(GetDx(btmPt1->Pt, p->Pt));
    p = btmPt2->Prev;
    while ((p->Pt == btmPt2->Pt) && (p != btmPt2))
    {
        p = p->Prev;
    }
    double dx2p = std::fabs(GetDx(btmPt2->Pt, p->Pt));
    p = btmPt2->Next;
    while ((p->Pt == btmPt2->Pt) && (p != btmPt2))
    {
        p = p->Next;
    }
    double dx2n = std::fabs(GetDx(btmPt2->Pt, p->Pt));

    if (std::max(dx1p, dx1n) == std::max(dx2p, dx2n) &&
        std::min(dx1p, dx1n) == std::min(dx2p, dx2n))
    {
        return Area(btmPt1) > 0; //if otherwise identical use orientation
    }
    else
    {
        return (dx1p >= dx2p && dx1p >= dx2n) || (dx1n >= dx2p && dx1n >= dx2n);
    }
}

template <typename T>
point_ptr<T> GetBottomPt(point_ptr<T> pp)
{
    point_ptr<T> dups = 0;
    point_ptr<T> p = pp->Next;
    while (p != pp)
    {
        if (p->Pt.y > pp->Pt.y)
        {
            pp = p;
            dups = 0;
        }
        else if (p->Pt.y == pp->Pt.y && p->Pt.x <= pp->Pt.x)
        {
            if (p->Pt.x < pp->Pt.x)
            {
                dups = 0;
                pp = p;
            }
            else
            {
                if (p->Next != pp && p->Prev != pp)
                {
                    dups = p;
                }
            }
        }
        p = p->Next;
    }
    if (dups)
    {
        //there appears to be at least 2 vertices at BottomPt so ...
        while (dups != p)
        {
            if (!FirstIsBottomPt(p, dups))
            {
                pp = dups;
            }
            dups = dups->Next;
            while (dups->Pt != pp->Pt)
            {
                dups = dups->Next;
            }
        }
    }
    return pp;
}

template <typename T>
bool Pt2IsBetweenPt1AndPt3(mapbox::geometry::point<T> pt1,
                           mapbox::geometry::point<T> pt2,
                           mapbox::geometry::point<T> pt3)
{
    if ((pt1 == pt3) || (pt1 == pt2) || (pt3 == pt2))
    {
        return false;
    }
    else if (pt1.x != pt3.x)
    {
        return (pt2.x > pt1.x) == (pt2.x < pt3.x);
    }
    else
    {
        return (pt2.y > pt1.y) == (pt2.y < pt3.y);
    }
}

template <typename T>
bool HorzSegmentsOverlap(T seg1a,
                         T seg1b,
                         T seg2a,
                         T seg2b)
{
    if (seg1a > seg1b)
    {
        std::swap(seg1a, seg1b);
    }
    if (seg2a > seg2b)
    {
        std::swap(seg2a, seg2b);
    }
    return (seg1a < seg2b) && (seg2a < seg1b);
}

}}}
