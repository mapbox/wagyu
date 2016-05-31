#pragma once

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/exceptions.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>

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
void RangeTest(mapnik::geometry::point<T> & Pt, bool & useFullRange)
{
    if (useFullRange)
    {
        if (Pt.X > hiRange || Pt.Y > hiRange || -Pt.X > hiRange || -Pt.Y > hiRange)
        {
            std::stringstream s;
            s << "Coordinate outside allowed range: ";
            s << std::fixed << Pt.X << " " << Pt.Y << " " << -Pt.X << " " << -Pt.Y;
            throw clipperException(s.str().c_str());
        }
    }
    else if (Pt.X > loRange|| Pt.Y > loRange || -Pt.X > loRange || -Pt.Y > loRange) 
    {
        useFullRange = true;
        RangeTest(Pt, useFullRange);
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
inline void InitEdge(edge_ptr<T> e, edge_ptr<T> eNext, edge_ptr<T> ePrev, mapbox::geometry::point<T> const& Pt)
{
    std::memset(e, 0, sizeof(edge<T>));
    e->Next = eNext;
    e->Prev = ePrev;
    e->Curr = Pt;
    e->OutIdx = Unassigned;
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
edge_ptr<T> ProcessBound(edge_ptr<T> E,
                         bool NextIsForward, 
                         minimum_list<T> & m_MinimaList)
{
    edge_ptr<T> Result = E;
    edge_ptr<T> Horz = nullptr;

    if (E->OutIdx == EDGE_SKIP)
    {
        //if edges still remain in the current bound beyond the skip edge then
        //create another LocMin and call ProcessBound once more
        if (NextIsForward)
        {
            while (E->Top.Y == E->Next->Bot.Y)
            {
                E = E->Next;
            }
            //don't include top horizontals when parsing a bound a second time,
            //they will be contained in the opposite bound ...
            while (E != Result && IsHorizontal(*E))
            {
                E = E->Prev;
            }
        }
        else
        {
            while (E->Top.Y == E->Prev->Bot.Y)
            {
                E = E->Prev;
            }
            while (E != Result && IsHorizontal(*E))
            {
                E = E->Next;
            }
        }

        if (E == Result)
        {
            if (NextIsForward)
            {
                Result = E->Next;
            }
            else 
            {
                Result = E->Prev;
            }
        }
        else
        {
            //there are more edges in the bound beyond result starting with E
            if (NextIsForward)
            {
                E = Result->Next;
            }
            else
            {
                E = Result->Prev;
            }
            local_minimum<T> locMin;
            locMin.Y = E->Bot.Y;
            locMin.LeftBound = nullptr;
            locMin.RightBound = E;
            E->WindDelta = 0;
            Result = ProcessBound(E, NextIsForward, m_MinimaList);
            m_MinimaList.push_back(locMin);
        }
        return Result;
    }

    edge_ptr<T> EStart = nullptr;

    if (IsHorizontal(*E))
    {
        //We need to be careful with open paths because this may not be a
        //true local minima (ie E may be following a skip edge).
        //Also, consecutive horz. edges may start heading left before going right.
        if (NextIsForward)
        {
            EStart = E->Prev;
        }
        else
        {
            EStart = E->Next;
        }
        if (IsHorizontal(*EStart)) //ie an adjoining horizontal skip edge
        {
            if (EStart->Bot.X != E->Bot.X && EStart->Top.X != E->Bot.X)
            {
                ReverseHorizontal(*E);
            }
        }
        else if (EStart->Bot.X != E->Bot.X)
        {
            ReverseHorizontal(*E);
        }
    }

    EStart = E;
    if (NextIsForward)
    {
        while (Result->Top.Y == Result->Next->Bot.Y && 
               Result->Next->OutIdx != EDGE_SKIP)
        {
            Result = Result->Next;
        }
        if (IsHorizontal(*Result) && Result->Next->OutIdx != EDGE_SKIP)
        {
            //nb: at the top of a bound, horizontals are added to the bound
            //only when the preceding edge attaches to the horizontal's left vertex
            //unless a Skip edge is encountered when that becomes the top divide
            Horz = Result;
            while (IsHorizontal(*Horz->Prev))
            {
                Horz = Horz->Prev;
            }
            if (Horz->Prev->Top.X > Result->Next->Top.X)
            {
                Result = Horz->Prev;
            }
        }
        while (E != Result) 
        {
            E->NextInLML = E->Next;
            if (IsHorizontal(*E) && 
                E != EStart &&
                E->Bot.X != E->Prev->Top.X)
            {
                ReverseHorizontal(*E);
            }
            E = E->Next;
        }
        if (IsHorizontal(*E) && E != EStart && 
            E->Bot.X != E->Prev->Top.X)
        {
            ReverseHorizontal(*E);
        }
        Result = Result->Next; //move to the edge just beyond current bound
    }
    else
    {
        while (Result->Top.Y == Result->Prev->Bot.Y && 
               Result->Prev->OutIdx != EDGE_SKIP) 
        {
            Result = Result->Prev;
        }
        if (IsHorizontal(*Result) && 
            Result->Prev->OutIdx != EDGE_SKIP)
        {
            Horz = Result;
            while (IsHorizontal(*Horz->Next))
            {
                Horz = Horz->Next;
            }
            if (Horz->Next->Top.X == Result->Prev->Top.X ||
                Horz->Next->Top.X > Result->Prev->Top.X)
            {
                Result = Horz->Next;
            }
        }

        while (E != Result)
        {
            E->NextInLML = E->Prev;
            if (IsHorizontal(*E) && 
                E != EStart && 
                E->Bot.X != E->Next->Top.X) 
            {
                ReverseHorizontal(*E);
            }
            E = E->Prev;
        }
        if (IsHorizontal(*E) && 
            E != EStart && 
            E->Bot.X != E->Next->Top.X) 
        {
            ReverseHorizontal(*E);
        }
        Result = Result->Prev; //move to the edge just beyond current bound
    }
    return Result;
}

template <typename T>
edge_ptr<T> FindNextLocMin(edge_ptr<T> E)
{
    for (;;)
    {
        while (E->Bot != E->Prev->Bot || E->Curr == E->Top)
        {
            E = E->Next;
        }
        if (!IsHorizontal(*E) && !IsHorizontal(*E->Prev))
        {
            break;
        }
        while (IsHorizontal(*E->Prev))
        {
            E = E->Prev;
        }
        edge_ptr<T> E2 = E;
        while (IsHorizontal(*E))
        {
            E = E->Next;
        }
        if (E->Top.Y == E->Prev->Bot.Y)
        {
            continue; //ie just an intermediate horz.
        }
        if (E2->Prev->Bot.X < E->Bot.X)
        {
            E = E2;
        }
        break;
    }
    return E;
}

template <typename T>
bool add_edge(std::vector<mapbox::geometry::point<T> > Path const& pg,
              edge_list<T> & m_edges,
              minimum_list<T> & m_MinimaList,
              polygon_type PolyTyp, 
              bool Closed,
              bool m_PreserveCollinear = false,
              bool m_UseFullRange = false)
{
#ifdef use_lines
    if (!Closed && PolyTyp == ptClip)
    {
        throw clipper_exception("AddPath: Open paths must be subject.");
    }
#else
    if (!Closed)
    {
        throw clipper_exception("AddPath: Open paths have been disabled.");
    }
#endif

    int highI = static_cast<int>(pg.size()) - 1;
    if (Closed)
    {
        while (highI > 0 && (pg[highI] == pg[0]))
        {
            --highI;
        }
    }
    while (highI > 0 && (pg[highI] == pg[highI - 1]))
    {
        --highI;
    }
    
    if ((Closed && highI < 2) || (!Closed && highI < 1))
    {
        return false;
    }

    //create a new edge array ...
    edge_ptr<T> edges = new TEdge [highI + 1];

    bool IsFlat = true;
    //1. Basic (first) edge initialization ...
    try
    {
        edges[1].Curr = pg[1];
        RangeTest(pg[0], m_UseFullRange);
        RangeTest(pg[highI], m_UseFullRange);
        InitEdge(&edges[0], &edges[1], &edges[highI], pg[0]);
        InitEdge(&edges[highI], &edges[0], &edges[highI - 1], pg[highI]);
        for (int i = highI - 1; i >= 1; --i)
        {
            RangeTest(pg[i], m_UseFullRange);
            InitEdge(&edges[i], &edges[i+1], &edges[i-1], pg[i]);
        }
    }
    catch (std::exception const&)
    {
        delete [] edges;
        throw; //range test fails
    }

    edge_ptr<T> eStart = &edges[0];

    //2. Remove duplicate vertices, and (when closed) collinear edges ...
    edge_ptr<T> E = eStart;
    edge_ptr<T> eLoopStop = eStart;
    for (;;)
    {
        //nb: allows matching start and end points when not Closed ...
        if (E->Curr == E->Next->Curr && 
            (Closed || E->Next != eStart))
        {
            if (E == E->Next)
            {
                break;
            }
            if (E == eStart)
            {
                eStart = E->Next;
            }
            E = RemoveEdge(E);
            eLoopStop = E;
            continue;
        }
        if (E->Prev == E->Next)
        {
            break; //only two vertices
        }
        else if (Closed &&
                 SlopesEqual(E->Prev->Curr, E->Curr, E->Next->Curr, m_UseFullRange) && 
                 (!m_PreserveCollinear ||
                 !Pt2IsBetweenPt1AndPt3(E->Prev->Curr, E->Curr, E->Next->Curr)))
        {
            //Collinear edges are allowed for open paths but in closed paths
            //the default is to merge adjacent collinear edges into a single edge.
            //However, if the PreserveCollinear property is enabled, only overlapping
            //collinear edges (ie spikes) will be removed from closed paths.
            if (E == eStart)
            {
                eStart = E->Next;
            }
            E = RemoveEdge(E);
            E = E->Prev;
            eLoopStop = E;
            continue;
        }
        E = E->Next;
        if ((E == eLoopStop) || (!Closed && E->Next == eStart))
        {
            break;
        }
    }

    if ((!Closed && (E == E->Next)) || (Closed && (E->Prev == E->Next)))
    {
        delete [] edges;
        return false;
    }

    if (!Closed)
    { 
        eStart->Prev->OutIdx = EDGE_SKIP;
    }

    //3. Do second stage of edge initialization ...
    E = eStart;
    do
    {
        InitEdge2(*E, PolyTyp);
        E = E->Next;
        if (IsFlat && E->Curr.Y != eStart->Curr.Y)
        {
            IsFlat = false;
        }
    }
    while (E != eStart);

    //4. Finally, add edge bounds to LocalMinima list ...

    //Totally flat paths must be handled differently when adding them
    //to LocalMinima list to avoid endless loops etc ...
    if (IsFlat) 
    {
        if (Closed) 
        {
            delete [] edges;
            return false;
        }
        E->Prev->OutIdx = EDGE_SKIP;
        local_minimum<T> locMin;
        locMin.Y = E->Bot.Y;
        locMin.LeftBound = nullptr;
        locMin.RightBound = E;
        locMin.RightBound->Side = edge_right;
        locMin.RightBound->WindDelta = 0;
        for (;;)
        {
            if (E->Bot.X != E->Prev->Top.X)
            {
                ReverseHorizontal(*E);
            }
            if (E->Next->OutIdx == EDGE_SKIP)
            {
                break;
            }
            E->NextInLML = E->Next;
            E = E->Next;
        }
        m_MinimaList.push_back(locMin);
        m_edges.push_back(edges);
        return true;
    }

    m_edges.push_back(edges);
    bool leftBoundIsForward;
    edge_ptr<T> EMin = 0;

    //workaround to avoid an endless loop in the while loop below when
    //open paths have matching start and end points ...
    if (E->Prev->Bot == E->Prev->Top)
    {
        E = E->Next;
    }

    for (;;)
    {
        E = FindNextLocMin(E);
        if (E == EMin)
        {
            break;
        }
        else if (!EMin)
        {
            EMin = E;
        }

        //E and E.Prev now share a local minima (left aligned if horizontal).
        //Compare their slopes to find which starts which bound ...
        local_minimum<T> locMin;
        locMin.Y = E->Bot.Y;
        if (E->Dx < E->Prev->Dx) 
        {
            locMin.LeftBound = E->Prev;
            locMin.RightBound = E;
            leftBoundIsForward = false; //Q.nextInLML = Q.prev
        }
        else
        {
            locMin.LeftBound = E;
            locMin.RightBound = E->Prev;
            leftBoundIsForward = true; //Q.nextInLML = Q.next
        }

        if (!Closed)
        {
            locMin.LeftBound->WindDelta = 0;
        }
        else if (locMin.LeftBound->Next == locMin.RightBound)
        {
            locMin.LeftBound->WindDelta = -1;
        }
        else
        {
            locMin.LeftBound->WindDelta = 1;
        }
        locMin.RightBound->WindDelta = -locMin.LeftBound->WindDelta;

        E = ProcessBound(locMin.LeftBound, leftBoundIsForward);
        if (E->OutIdx == EDGE_SKIP)
        {
            E = ProcessBound(E, leftBoundIsForward);
        }

        edge_ptr<T> E2 = ProcessBound(locMin.RightBound, !leftBoundIsForward);
        if (E2->OutIdx == EDGE_SKIP)
        {
            E2 = ProcessBound(E2, !leftBoundIsForward);
        }

        if (locMin.LeftBound->OutIdx == EDGE_SKIP)
        {
            locMin.LeftBound = nullptr;
        }
        else if (locMin.RightBound->OutIdx == EDGE_SKIP)
        {
            locMin.RightBound = 0;
        }
        m_MinimaList.push_back(locMin);
        if (!leftBoundIsForward)
        {
            E = E2;
        }
    }
    return true;
}

}}}
