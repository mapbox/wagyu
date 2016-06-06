#pragma once

#include <list>
#include <queue>

#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/ring/ring.hpp>

#include <mapbox/geometry/wagyu/box.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/local_minimum.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/polytree.hpp>
#include <mapbox/geometry/wagyu/ring.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
class clipper
{
private:
    using value_type = T;
    using scanbeam_list = std::priority_queue<value_type>;
    using maxima_list = std::list<value_type>;
    
    local_minimum_list<value_type>           m_MinimaList;
    typename local_minimum_list<value_type>::iterator m_CurrentLM;
    std::vector<edge_list<value_type> > m_edges;
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
        m_ClipFillType(fill_type_even_odd),
        m_SubjFillType(fill_type_even_odd),
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
    
    bool add_line(mapbox::geometry::line_string<value_type> const& pg)
    {
        bool success = add_line_string(pg, m_edges, m_MinimaList);
        if (success)
        {
            m_HasOpenPaths = true;
        }
        return success;
    }

    bool add_ring(mapbox::geometry::linear_ring<value_type> const& pg, 
                  polygon_type p_type = polygon_type_subject)
    {
        return add_linear_ring(pg, m_edges, m_MinimaList, p_type);
    }

    bool add_polygon(mapbox::geometry::polygon<value_type> const& ppg, 
                     polygon_type p_type = polygon_type_subject)
    {
        bool result = false;
        for (std::size_t i = 0; i < ppg.size(); ++i)
        {
            if (add_ring(ppg[i], p_type))
            {
                result = true;
            }
        }
        return result;
    }

    void clear()
    {
        m_MinimaList.clear();
        m_CurrentLM = m_MinimaList.begin();
        m_edges.clear();
        m_HasOpenPaths = false;
    }

    void reset()
    {
        m_CurrentLM = m_MinimaList.begin();
        if (m_CurrentLM == m_MinimaList.end())
        {
            return; //ie nothing to process
        }
        std::stable_sort(m_MinimaList.begin(), m_MinimaList.end(), local_minimum_sorter<value_type>());

        m_Scanbeam = scanbeam_list(); //clears/resets priority_queue
        //reset all edges ...
        for (auto const& lm = m_MinimaList.begin(); lm != m_MinimaList.end(); ++lm)
        {
            m_Scanbeam.push(lm->Y);
            edge_ptr<value_type> e = lm->LeftBound;
            if (e)
            {
                e->Curr = e->Bot;
                e->Side = edge_left;
                e->OutIdx = EDGE_UNASSIGNED;
            }

            e = lm->RightBound;
            if (e)
            {
                e->Curr = e->Bot;
                e->Side = edge_right;
                e->OutIdx = EDGE_UNASSIGNED;
            }
        }
        m_ActiveEdges = 0;
        m_CurrentLM = m_MinimaList.begin();
    }

    box<value_type> get_bounds()
    {
        box<value_type> result = { 0, 0, 0, 0 };
        auto lm = m_MinimaList.begin();
        if (lm == m_MinimaList.end())
        {
            return result;
        }
        result.left = lm->LeftBound->Bot.x;
        result.top = lm->LeftBound->Bot.y;
        result.right = lm->LeftBound->Bot.x;
        result.bottom = lm->LeftBound->Bot.y;
        while (lm != m_MinimaList.end())
        {
            //todo - needs fixing for open paths
            result.bottom = std::max(result.bottom, lm->LeftBound->Bot.y);
            edge_ptr<value_type> e = lm->LeftBound;
            for (;;)
            {
                edge_ptr<value_type> bottomE = e;
                while (e->NextInLML)
                {
                    if (e->Bot.x < result.left)
                    {
                        result.left = e->Bot.x;
                    }
                    if (e->Bot.x > result.right)
                    {
                        result.right = e->Bot.x;
                    }
                    e = e->NextInLML;
                }
                result.left = std::min(result.left, e->Bot.x);
                result.right = std::max(result.right, e->Bot.x);
                result.left = std::min(result.left, e->Top.x);
                result.right = std::max(result.right, e->Top.x);
                result.top = std::min(result.top, e->Top.y);
                if (bottomE == lm->LeftBound)
                {
                    e = lm->RightBound;
                }
                else
                {
                    break;
                }
            }
            ++lm;
        }
        return result;
    }
    
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
  
    /*
    bool Execute(clip_type clipType,
                 linear_ring_list<value_type> &solution,
                 fill_type subjFillType,
                 fill_type clipFillType)
    {

    }
    
    bool Execute(clip_type clipType,
                 polygon_tree<value_type> & polytree,
                 fill_type subjFillType,
                 fill_type clipFillType)
    {

    }
    */
private:
    
    /*
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
    */

};

}}}
