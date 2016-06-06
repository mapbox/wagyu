#pragma once

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
inline void ReverseHorizontal(edge<T> & e)
{
    //swap horizontal edges' Top and Bottom x's so they follow the natural
    //progression of the bounds - ie so their xbots will align with the
    //adjoining lower edge. [Helpful in the ProcessHorizontal() method.]
    std::swap(e.Top.x, e.Bot.x);
}

template <typename T>
edge_ptr<T> RemoveEdge(edge_ptr<T> e)
{
    //removes e from double_linked_list (but without removing from memory)
    e->Prev->Next = e->Next;
    e->Next->Prev = e->Prev;
    edge_ptr<T> result = e->Next;
    e->Prev = nullptr; //flag as removed (see ClipperBase.Clear)
    return result;
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
inline void InitEdge(edge_ptr<T> e, edge_ptr<T> eNext, edge_ptr<T> ePrev, mapbox::geometry::point<T> const& Pt)
{
    std::memset(e, 0, sizeof(edge<T>));
    e->Next = eNext;
    e->Prev = ePrev;
    e->Curr = Pt;
    e->OutIdx = EDGE_UNASSIGNED;
}

template <typename T>
void InitEdge2(edge<T> & e, polygon_type Pt)
{
    if (e.Curr.y >= e.Next->Curr.y)
    {
        e.Bot = e.Curr;
        e.Top = e.Next->Curr;
    }
    else
    {
        e.Top = e.Curr;
        e.Bot = e.Next->Curr;
    }
    SetDx(e);
    e.PolyTyp = Pt;
}

template <typename T>
edge_ptr<T> process_bound_type_line(edge_ptr<T> current_edge,
                                    bool next_is_forward, 
                                    local_minimum_list<T> & minima_list)
{
    edge_ptr<T> result = current_edge;
    edge_ptr<T> horizontal_edge = nullptr;

    if (current_edge->OutIdx == EDGE_SKIP)
    {
        //if edges still remain in the current bound beyond the skip edge then
        //create another LocMin and call ProcessBound once more
        if (next_is_forward)
        {
            while (current_edge->Top.y == current_edge->Next->Bot.y)
            {
                current_edge = current_edge->Next;
            }
            //don't include top horizontals when parsing a bound a second time,
            //they will be contained in the opposite bound ...
            while (current_edge != result && 
                   IsHorizontal(*current_edge))
            {
                current_edge = current_edge->Prev;
            }
        }
        else
        {
            while (current_edge->Top.y == current_edge->Prev->Bot.y)
            {
                current_edge = current_edge->Prev;
            }
            while (current_edge != result && 
                   IsHorizontal(*current_edge))
            {
                current_edge = current_edge->Next;
            }
        }

        if (current_edge == result)
        {
            if (next_is_forward)
            {
                result = current_edge->Next;
            }
            else 
            {
                result = current_edge->Prev;
            }
        }
        else
        {
            //there are more edges in the bound beyond result starting with current_edge
            if (next_is_forward)
            {
                current_edge = result->Next;
            }
            else
            {
                current_edge = result->Prev;
            }
            local_minimum<T> locMin;
            locMin.y = current_edge->Bot.y;
            locMin.LeftBound = nullptr;
            locMin.RightBound = current_edge;
            current_edge->WindDelta = 0;
            result = process_bound_type_line(current_edge, next_is_forward, minima_list);
            minima_list.push_back(locMin);
        }
        return result;
    }

    edge_ptr<T> starting_edge = nullptr;

    if (IsHorizontal(*current_edge))
    {
        //We need to be careful with open paths because this may not be a
        //true local minima (ie current_edge may be following a skip edge).
        //Also, consecutive horz. edges may start heading left before going right.
        if (next_is_forward)
        {
            starting_edge = current_edge->Prev;
        }
        else
        {
            starting_edge = current_edge->Next;
        }
        if (IsHorizontal(*starting_edge)) //ie an adjoining horizontal skip edge
        {
            if (starting_edge->Bot.x != current_edge->Bot.x && starting_edge->Top.x != current_edge->Bot.x)
            {
                ReverseHorizontal(*current_edge);
            }
        }
        else if (starting_edge->Bot.x != current_edge->Bot.x)
        {
            ReverseHorizontal(*current_edge);
        }
    }

    starting_edge = current_edge;
    if (next_is_forward)
    {
        while (result->Top.y == result->Next->Bot.y && 
               result->Next->OutIdx != EDGE_SKIP)
        {
            result = result->Next;
        }
        if (IsHorizontal(*result) && 
            result->Next->OutIdx != EDGE_SKIP)
        {
            //nb: at the top of a bound, horizontals are added to the bound
            //only when the preceding edge attaches to the horizontal's left vertex
            //unless a Skip edge is encountered when that becomes the top divide
            horizontal_edge = result;
            while (IsHorizontal(*horizontal_edge->Prev))
            {
                horizontal_edge = horizontal_edge->Prev;
            }
            if (horizontal_edge->Prev->Top.x > result->Next->Top.x)
            {
                result = horizontal_edge->Prev;
            }
        }
        while (current_edge != result) 
        {
            current_edge->NextInLML = current_edge->Next;
            if (IsHorizontal(*current_edge) && 
                current_edge != starting_edge &&
                current_edge->Bot.x != current_edge->Prev->Top.x)
            {
                ReverseHorizontal(*current_edge);
            }
            current_edge = current_edge->Next;
        }
        if (IsHorizontal(*current_edge) &&
            current_edge != starting_edge && 
            current_edge->Bot.x != current_edge->Prev->Top.x)
        {
            ReverseHorizontal(*current_edge);
        }
        result = result->Next; //move to the edge just beyond current bound
    }
    else
    {
        while (result->Top.y == result->Prev->Bot.y && 
               result->Prev->OutIdx != EDGE_SKIP) 
        {
            result = result->Prev;
        }
        if (IsHorizontal(*result) && 
            result->Prev->OutIdx != EDGE_SKIP)
        {
            horizontal_edge = result;
            while (IsHorizontal(*horizontal_edge->Next))
            {
                horizontal_edge = horizontal_edge->Next;
            }
            if (horizontal_edge->Next->Top.x >= result->Prev->Top.x)
            {
                result = horizontal_edge->Next;
            }
        }

        while (current_edge != result)
        {
            current_edge->NextInLML = current_edge->Prev;
            if (IsHorizontal(*current_edge) && 
                current_edge != starting_edge && 
                current_edge->Bot.x != current_edge->Next->Top.x) 
            {
                ReverseHorizontal(*current_edge);
            }
            current_edge = current_edge->Prev;
        }
        if (IsHorizontal(*current_edge) && 
            current_edge != starting_edge && 
            current_edge->Bot.x != current_edge->Next->Top.x) 
        {
            ReverseHorizontal(*current_edge);
        }
        result = result->Prev; //move to the edge just beyond current bound
    }
    return result;
}

template <typename T>
edge_ptr<T> process_bound_type_ring(edge_ptr<T> current_edge,
                                    bool next_is_forward)
{
    edge_ptr<T> result = current_edge;
    edge_ptr<T> horizontal_edge = nullptr;
    edge_ptr<T> starting_edge = nullptr;

    if (IsHorizontal(*current_edge))
    {
        // We need to be careful with open paths because this may not be a
        // true local minima (ie E may be following a skip edge).
        // Also, consecutive horz. edges may start heading left before going right.
        if (next_is_forward)
        {
            starting_edge = current_edge->Prev;
        }
        else
        {
            starting_edge = current_edge->Next;
        }
        if (IsHorizontal(*starting_edge)) //ie an adjoining horizontal skip edge
        {
            if (starting_edge->Bot.x != current_edge->Bot.x && starting_edge->Top.x != current_edge->Bot.x)
            {
                ReverseHorizontal(*current_edge);
            }
        }
        else if (starting_edge->Bot.x != current_edge->Bot.x)
        {
            ReverseHorizontal(*current_edge);
        }
    }

    starting_edge = current_edge;
    if (next_is_forward)
    {
        while (result->Top.y == result->Next->Bot.y)
        {
            result = result->Next;
        }
        if (IsHorizontal(*result))
        {
            //nb: at the top of a bound, horizontals are added to the bound
            //only when the preceding edge attaches to the horizontal's left vertex
            //unless a Skip edge is encountered when that becomes the top divide
            horizontal_edge = result;
            while (IsHorizontal(*horizontal_edge->Prev))
            {
                horizontal_edge = horizontal_edge->Prev;
            }
            if (horizontal_edge->Prev->Top.x > result->Next->Top.x)
            {
                result = horizontal_edge->Prev;
            }
        }
        while (current_edge != result) 
        {
            current_edge->NextInLML = current_edge->Next;
            if (IsHorizontal(*current_edge) && 
                current_edge != starting_edge &&
                current_edge->Bot.x != current_edge->Prev->Top.x)
            {
                ReverseHorizontal(*current_edge);
            }
            current_edge = current_edge->Next;
        }
        if (IsHorizontal(*current_edge) && 
            current_edge != starting_edge && 
            current_edge->Bot.x != current_edge->Prev->Top.x)
        {
            ReverseHorizontal(*current_edge);
        }
        result = result->Next; //move to the edge just beyond current bound
    }
    else
    {
        while (result->Top.y == result->Prev->Bot.y)
        {
            result = result->Prev;
        }
        if (IsHorizontal(*result))
        {
            horizontal_edge = result;
            while (IsHorizontal(*horizontal_edge->Next))
            {
                horizontal_edge = horizontal_edge->Next;
            }
            if (horizontal_edge->Next->Top.x >= result->Prev->Top.x)
            {
                result = horizontal_edge->Next;
            }
        }

        while (current_edge != result)
        {
            current_edge->NextInLML = current_edge->Prev;
            if (IsHorizontal(*current_edge) && 
                current_edge != starting_edge && 
                current_edge->Bot.x != current_edge->Next->Top.x) 
            {
                ReverseHorizontal(*current_edge);
            }
            current_edge = current_edge->Prev;
        }
        if (IsHorizontal(*current_edge) && 
            current_edge != starting_edge && 
            current_edge->Bot.x != current_edge->Next->Top.x) 
        {
            ReverseHorizontal(*current_edge);
        }
        result = result->Prev; //move to the edge just beyond current bound
    }
    return result;
}

template <typename T>
edge_ptr<T> find_next_local_minimum(edge_ptr<T> edge)
{
    while (true)
    {
        while (edge->Bot != edge->Prev->Bot || edge->Curr == edge->Top)
        {
            edge = edge->Next;
        }
        if (!IsHorizontal(*edge) && !IsHorizontal(*edge->Prev))
        {
            break;
        }
        while (IsHorizontal(*edge->Prev))
        {
            edge = edge->Prev;
        }
        edge_ptr<T> edge2 = edge;
        while (IsHorizontal(*edge))
        {
            edge = edge->Next;
        }
        if (edge->Top.y == edge->Prev->Bot.y)
        {
            continue; //ie just an intermediate horz.
        }
        if (edge2->Prev->Bot.x < edge->Bot.x)
        {
            edge = edge2;
        }
        break;
    }
    return edge;
}

template  <typename T>
void add_flat_line_to_local_minima_list(edge_list<T> & new_edges,
                                        local_minimum_list<T> & minima_list)
{
    using value_type = T;
    // Totally flat paths must be handled differently when adding them
    // to LocalMinima list to avoid endless loops etc ...
    edge_ptr<value_type> current_edge = &new_edges.back();
    current_edge->Prev->OutIdx = EDGE_SKIP;
    local_minimum<value_type> local_min;
    local_min.y = current_edge->Bot.y;
    local_min.LeftBound = nullptr;
    local_min.RightBound = current_edge;
    local_min.RightBound->Side = edge_right;
    local_min.RightBound->WindDelta = 0;
    for (;;)
    {
        if (current_edge->Bot.x != current_edge->Prev->Top.x)
        {
            ReverseHorizontal(*current_edge);
        }
        if (current_edge->Next->OutIdx == EDGE_SKIP)
        {
            break;
        }
        current_edge->NextInLML = current_edge->Next;
        current_edge = current_edge->Next;
    }
    minima_list.push_back(local_min);
}

template  <typename T>
void add_line_to_local_minima_list(edge_list<T> & new_edges,
                                   local_minimum_list<T> & minima_list)
{
    using value_type = T;
    bool left_bound_is_forward = false;
    edge_ptr<value_type> starting_edge = &new_edges.back();
    edge_ptr<value_type> current_edge = starting_edge;
    edge_ptr<value_type> minimum_edge = nullptr;

    for (;;)
    {
        current_edge = find_next_local_minimum(current_edge);
        if (current_edge == minimum_edge)
        {
            break;
        }
        else if (minimum_edge == nullptr)
        {
            minimum_edge = current_edge;
        }

        //E and E.Prev now share a local minima (left aligned if horizontal).
        //Compare their slopes to find which starts which bound ...
        local_minimum<value_type> local_min;
        local_min.y = current_edge->Bot.y;
        if (current_edge->Dx < current_edge->Prev->Dx) 
        {
            local_min.LeftBound = current_edge->Prev;
            local_min.RightBound = current_edge;
            left_bound_is_forward = false; //Q.nextInLML = Q.prev
        }
        else
        {
            local_min.LeftBound = current_edge;
            local_min.RightBound = current_edge->Prev;
            left_bound_is_forward = true; //Q.nextInLML = Q.next
        }

        local_min.LeftBound->WindDelta = 0;
        local_min.RightBound->WindDelta = 0;

        current_edge = process_bound_type_line(local_min.LeftBound, left_bound_is_forward, minima_list);
        if (current_edge->OutIdx == EDGE_SKIP) 
        {
            current_edge = process_bound_type_line(current_edge, left_bound_is_forward, minima_list);
        }

        edge_ptr<value_type> current_edge_2 = process_bound_type_line(local_min.RightBound, !left_bound_is_forward, minima_list);
        if (current_edge_2->OutIdx == EDGE_SKIP) 
        {
            current_edge_2 = process_bound_type_line(current_edge_2, !left_bound_is_forward, minima_list);
        }

        minima_list.push_back(local_min);
        
        if (!left_bound_is_forward)
        {
            current_edge = current_edge_2;
        }
    }
}

template  <typename T>
void add_ring_to_local_minima_list(edge_list<T> & new_edges,
                                   local_minimum_list<T> & minima_list)
{
    using value_type = T;
    bool left_bound_is_forward = false;
    edge_ptr<value_type> starting_edge = &new_edges.back();
    edge_ptr<value_type> current_edge = starting_edge;
    edge_ptr<value_type> minimum_edge = nullptr;

    for (;;)
    {
        current_edge = find_next_local_minimum(current_edge);
        if (current_edge == minimum_edge)
        {
            break;
        }
        else if (minimum_edge == nullptr)
        {
            minimum_edge = current_edge;
        }

        //E and E.Prev now share a local minima (left aligned if horizontal).
        //Compare their slopes to find which starts which bound ...
        local_minimum<value_type> local_min;
        local_min.y = current_edge->Bot.y;
        if (current_edge->Dx < current_edge->Prev->Dx) 
        {
            local_min.LeftBound = current_edge->Prev;
            local_min.RightBound = current_edge;
            left_bound_is_forward = false; //Q.nextInLML = Q.prev
        }
        else
        {
            local_min.LeftBound = current_edge;
            local_min.RightBound = current_edge->Prev;
            left_bound_is_forward = true; //Q.nextInLML = Q.next
        }

        if (local_min.LeftBound->Next == local_min.RightBound)
        {
            local_min.LeftBound->WindDelta = -1;
            local_min.RightBound->WindDelta = 1;
        }
        else
        {
            local_min.LeftBound->WindDelta = 1;
            local_min.RightBound->WindDelta = -1;
        }

        current_edge = process_bound_type_ring(local_min.LeftBound, left_bound_is_forward);

        edge_ptr<value_type> current_edge_2 = process_bound_type_ring(local_min.RightBound, !left_bound_is_forward);

        minima_list.push_back(local_min);
        
        if (!left_bound_is_forward)
        {
            current_edge = current_edge_2;
        }
    }
}

template <typename T>
void make_list_circular(edge_list<T> & edges)
{
    // Link all edges for circular list now
    edges.front().Prev = &edges.back();
    edges.back().Next = &edges.front();
    auto itr_next = edges.begin();
    auto itr = itr_next++;
    for (; itr_next != edges.end(); ++itr, ++itr_next)
    {
        itr->Next = &(*itr_next);
        itr_next->Prev = &(*itr);
    }
}

template <typename T>
bool build_edge_list(mapbox::geometry::line_string<T> const& path_geometry,
                     edge_list<T> & edges,
                     bool & is_flat)
{
    if (path_geometry.size() < 2)
    {
        return false;
    }
    
    auto itr_next = path_geometry.begin();
    ++itr_next;
    auto itr = path_geometry.begin();
    while (itr_next != path_geometry.end())
    {
        if (*itr_next == *itr)
        {
            // Duplicate point advance itr_next, but do not
            // advance itr
            ++itr_next;
            continue;
        }

        if (is_flat && itr_next->y != itr->y)
        {
            is_flat = false;
        }
        edges.emplace_back(*itr, *itr_next, polygon_type_subject);
        
        itr = itr_next;
        ++itr_next;
    }
    
    if (edges.size() < 2)
    {
        return false;
    }

    return true;
}

template <typename T>
bool add_line_string(mapbox::geometry::line_string<T> const& path_geometry,
              edge_list<T> & edges,
              local_minimum_list<T> & minima_list)
{
    bool is_flat = true;
    edges.push_back();
    auto & new_edges = edges.back();
    if (!build_edge_list(path_geometry, new_edges, is_flat) || new_edges.empty())
    {
        edges.pop_back();
        return false;
    }
    
    make_list_circular(new_edges);
   
    if (is_flat) 
    {
        add_flat_line_to_local_minima_list(new_edges, minima_list);
    }
    else
    {
        add_line_to_local_minima_list(new_edges, minima_list);
    }
    return true;
}

template <typename T>
bool build_edge_list(mapbox::geometry::linear_ring<T> const& path_geometry,
                     edge_list<T> & edges,
                     polygon_type p_type)
{
    using value_type = T;

    if (path_geometry.size() < 3)
    {
        return false;
    }
    
    // As this is a loop, we need to first go backwards from end to try and find
    // the proper starting point for the iterators before the beginning
    
    auto itr_rev = path_geometry.end();
    auto itr = path_geometry.begin();
    --itr_rev;
    mapbox::geometry::point<value_type> pt1 = *itr_rev;
    mapbox::geometry::point<value_type> pt2 = *itr_rev;
    mapbox::geometry::point<value_type> pt3 = *itr;
    for (;;)
    {
        // Find next non repeated point going backwards from end of ring
        while (pt2 == pt3)
        {
            pt2 = *(--itr_rev);
            if (itr_rev == path_geometry.begin())
            {
                return false;
            }
        }

        // Once more, find next non repeated point going backwards but
        // this time from itr_next_1
        do
        {
            pt1 = *(--itr_rev);
            if (itr_rev == path_geometry.begin())
            {
                return false;
            }
        }
        while (pt1 == pt2);
        
        if (SlopesEqual(pt1, pt2, pt3))
        {
            pt2 = pt1;
        }
        else
        {
            break;
        }
    }
    
    mapbox::geometry::point<value_type> pt_first = pt1;
    
    while (true)
    {
        if (pt3 == pt2)
        {
            // Duplicate point advance itr, but do not
            // advance other points
            ++itr;
            if (itr == path_geometry.end())
            {
                break;
            }
            pt3 = *itr;
            continue;
        }

        // Now check if slopes are equal between two segments - either
        // a spike or a collinear point - if so drop point number 2.
        if (SlopesEqual(pt1, pt2, pt3))
        {
            // We need to reconsider previously added points
            // because the point it was using was found to be collinear
            // or a spike
            pt2 = pt1;
            if (!edges.empty())
            {
                edges.pop_back(); // remove previous edge (pt1)
            }
            if (!edges.empty())
            {
                pt1 = edges.back().Curr;
            }
            else
            {
                pt1 = pt_first;
            }
            continue;
        }

        edges.emplace_back(pt2, pt3, p_type);
        ++itr;
        if (itr == path_geometry.end())
        {
            break;
        }
        pt1 = pt2;
        pt2 = pt3;
        pt3 = *itr;
    }

    if (edges.size() < 3)
    {
        return false;
    }
    return true;
}

template <typename T>
bool add_linear_ring(mapbox::geometry::linear_ring<T> const& path_geometry,
                     std::vector<edge_list<T> > & edges,
                     local_minimum_list<T> & minima_list,
                     polygon_type p_type)
{
    edges.emplace_back();
    auto & new_edges = edges.back();
    if (!build_edge_list(path_geometry, new_edges, p_type) || new_edges.empty())
    {
        edges.pop_back();
        return false;
    }
    make_list_circular(new_edges);
    add_ring_to_local_minima_list(new_edges, minima_list);
    return true;
}

}}}
