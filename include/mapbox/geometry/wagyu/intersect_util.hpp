#pragma once

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox
{
namespace geometry
{
namespace wagyu
{
static bool intersect_list_sort(intersect_list<value_type> * node1,
                                intersect_list<value_type> * node2)
{
    if (node2->pt.y != node1->pt.y)
    {
        return node2->pt.Y < node1->pt.Y;
    }
    else
    {
        return (node2->edge1->winding_count2 + node2->edge2->winding_count2) >
               (node1->edge1->winding_count2 + node1->edge2->winding_count2);
    }
}

void copy_AEL_to_SEL()
{
    edge<value_type> * e = m_ActiveEdges;
    m_SortedEdges = e;
    while (e) {
        e->prev_in_SEL = e->prev_in_AEL;
        e->next_in_SEL = e->next_in_SEL;
        e = e->next_in_AEL;
    }
}

void swap_positions_in_SEL(edge<value_type> * edge1, edge<value_type> * edge2)
{
    if (!(edge1->NextInSEL) && !(edge1->PrevInSEL))
    {
        return;
    }
    if (!(edge2->NextInSEL) && !(edge2->PrevInSEL))
    {
        return;
    }

    if (edge1->NextInSEL == edge2)
    {
        edge<value_type> * next = edge2->NextInSEL;
        if (next)
        {
            next->PrevInSEL = edge1;
        }
        edge<value_type> * prev = edge1->PrevInSEL;
        if (prev)
        {
            prev->NextInSEL = edge2;
        }
        edge2->PrevInSEL = prev;
        edge2->NextInSEL = edge1;
        edge1->PrevInSEL = edge2;
        edge1->NextInSEL = next;
    }
    else if (edge2->NextInSEL == edge1)
    {
        edge<value_type> * next = edge1->NextInSEL;
        if (next)
        {
            next->PrevInSEL = edge2;
        }
        edge<value_type> * prev = edge2->PrevInSEL;
        if (prev)
        {
            prev->NextInSEL = edge1;
        }
        edge1->PrevInSEL = prev;
        edge1->NextInSEL = edge2;
        edge2->PrevInSEL = edge1;
        edge2->NextInSEL = next;
    }
    else
    {
        edge<value_type> * next = edge1->NextInSEL;
        edge<value_type> * prev = edge1->PrevInSEL;
        edge1->NextInSEL = edge2->NextInSEL;
        if (edge1->NextInSEL)
        {
            edge1->NextInSEL->PrevInSEL = edge1;
        }
        edge1->PrevInSEL = edge2->PrevInSEL;
        if (edge1->PrevInSEL)
        {
            edge1->PrevInSEL->NextInSEL = edge1;
        }
        edge2->NextInSEL = next;
        if (edge2->NextInSEL)
        {
            edge2->NextInSEL->PrevInSEL = edge2;
        }
        edge2->PrevInSEL = prev;
        if (edge2->PrevInSEL)
        {
            edge2->PrevInSEL->NextInSEL = edge2;
        }
    }

    if (!edge1->PrevInSEL)
    {
        m_SortedEdges = edge1;
    }
    else if (!edge2->PrevInSEL)
    {
        m_SortedEdges = edge2;
    }
}

bool fixup_intersection_order()
{
    // Precondition: Intersections are sorted bottom-most first.
    // It's crucial that intersections are only made between adjacent edges,
    // so reorder the intersections to ensure this if necessary.

    copy_AEL_to_SEL();
    std::stable_sort(m_IntersectList.begin(), m_IntersectList.end(),
                     intersect_list_sort);

    size_t n = m_IntersectList.size();
    for (size_t i = 0; i < n; ++i) {
        if (!edges_adjacent(*m_IntersectList[i]))
        {
            size_t j = i + 1;
            while (j < n && !edges_adjacent(*m_IntersectList[j])) {
                j++;
            }
            if (j == n)
            {
                // Intersection could not be made adjacent
                return false;
            }
            std::swap(m_IntersectList[i], m_IntersectList[j]);
        }
        swap_positions_in_SEL(m_IntersectList[i]->edge1,
                              m_IntersectList[i]->edge2);
    }
    return true;
}

bool process_intersections(value_type top_y)
{
    if (!m_ActiveEdges)
    {
        return true;
    }

    build_intersect_list(top_y);

    size_t s = m_IntersectList.size();
    if (s == 0)
    {
        return true;
    }
    else if (s == 1 || fixup_intersection_order())
    {
        return true;
    }
    else
    {
        return false;
    }
}
}
}
}
