#pragma once

#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/ring/ring.hpp>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/polytree.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
using linear_ring_list = std::vector<mapbox::geometry::linear_ring<T> >;

template <typename T>
class clipper
{
private:
    using value_type = T;
    using scanbeam_list = std::priority_queue<value_type>;
    using maxima_list = std::list<value_type>;
    
    minimum_list<value_type>           m_MinimaList;
    minimum_list<value_type>::iterator m_CurrentLM;
    edge_list<value_type>              m_edges;
    ring_list<value_type>              m_PolyOuts;
    scanbeam_list                      m_Scanbeam;
    join_list<value_type>              m_Joins;
    join_list<value_type>              m_GhostJoins;
    intersect_list<value_type>         m_IntersectList;
    maxima_list                        m_Maxima;
    clip_type                          m_ClipType;
    edge_ptr<value_type>               m_ActiveEdges;
    edge_ptr<value_type>               m_SortedEdges;
    fill_type                          m_ClipFillType;
    fill_type                          m_SubjFillType;
    bool                               m_UseFullRange;
    bool                               m_PreserveCollinear;
    bool                               m_HasOpenPaths;
    bool                               m_ExecuteLocked;
    bool                               m_ReverseOutput;
    bool                               m_UsingPolyTree; 
    bool                               m_StrictSimple;

public:
    
    clipper() : 
        m_MinimaList(),
        m_CurrentLM(m_MinimaList.begin()),
        m_edges(),
        m_PolyOuts(),
        m_Scanbeam(),
        m_Joins(),
        m_GhostJoins(),
        m_IntersectList(),
        m_Maxima(),
        m_ActiveEdges(nullptr),
        m_SortedEdges(nullptr),
        m_ClipFillType(clip_type_intersection),
        m_SubjFillType(clip_type_intersection),
        m_UseFullRange(false),
        m_PreserveCollinear(false),
        m_HasOpenPaths(false),
        m_ExecuteLocked(false),
        m_ReverseOutput(false),
        m_UsingPolyTree(false),
        m_StrictSimple(false)
    {
    }
    
    ~clipper()
    {
        clear();
    }

    bool add_path(mapbox::geometry::linear_ring<value_type> const& pg, polygon_type PolyTyp, bool Closed)
    {
        bool success = add_edge(pg, PolyTyp, Closed, m_PreserveCollinear, m_UseFullRange);
        if (!Closed && success)
        {
            m_HasOpenPaths = true;
        }
    }

    bool add_paths(linear_ring_list<value_type> const& ppg, polygon_type PolyTyp, bool Closed)
    {
        bool result = false;
        for (std::size_t i = 0; i < ppg.size(); ++i)
        {
            if (add_path(ppg[i], PolyTyp, Closed))
            {
                result = true;
            }
        }
        return result;
    }

    void clear();
    IntRect get_bounds();
    
    bool preserve_collinear() 
    {
        return m_PreserveCollinear;
    }

    void preserve_collinear(bool value)
    {
        m_PreserveCollinear = value;
    }
    
    bool strictly_simple() 
    {
        return m_StrictSimple;
    }

    void strictly_simple(bool value)
    {
        m_StrictSimple = value;
    }
    
    bool reverse_output() 
    {
        return m_ReverseOutput;
    }

    void reverse_output(bool value)
    {
        m_ReverseOutput = value;
    }
  
    bool Execute(clip_type clipType,
                 linear_ring_list<value_type> &solution,
                 fill_type subjFillType,
                 fill_type clipFillType)
    {

    }
    
    bool Execute(clip_type clipType,
                 polygon_tree &polytree,
                 fill_type subjFillType,
                 fill_type clipFillType)
    {

    }

private:
    
    void dispose_local_minima_list()
    {

    }

    TEdge* AddBoundsToLML(TEdge *e, bool IsClosed);
    virtual void Reset();
    TEdge* ProcessBound(TEdge* E, bool IsClockwise);
    void InsertScanbeam(const cInt Y);
    bool PopScanbeam(cInt &Y);
    bool LocalMinimaPending();
    bool PopLocalMinima(cInt Y, const LocalMinimum *&locMin);
    OutRec* CreateOutRec();
    void DisposeAllOutRecs();
    void DisposeOutRec(PolyOutList::size_type index);
    void SwapPositionsInAEL(TEdge *edge1, TEdge *edge2);
    void DeleteFromAEL(TEdge *e);
    void UpdateEdgeIntoAEL(TEdge *&e);

};

}}}
