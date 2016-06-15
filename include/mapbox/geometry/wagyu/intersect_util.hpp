#pragma once

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox
{
namespace geometry
{
namespace wagyu
{

struct intersect_list_sorter
{
    template <typename T>
    inline bool intersect_list_sort(intersect_ptr<T> node1,
                                    intersect_ptr<T> node2)
    {
        if (node2->pt.y != node1->pt.y)
        {
            return node2->pt.y < node1->pt.y;
        }
        else
        {
            return (node2->edge1->winding_count2 + node2->edge2->winding_count2) >
                   (node1->edge1->winding_count2 + node1->edge2->winding_count2);
        }
    }
};

template <typename T>
bool edges_adjacent(intersect<T> const& inode)
{
    return (inode.edge1->next_in_SEL == inode.edge2) ||
           (inode.edge1->prev_in_SEL == inode.edge2);
}

template <typename T>
bool fixup_intersection_order(edge_ptr<T> active_edge_list,
                              edge_ptr<T> & sorted_edge_list,
                              intersect_list<T> & intersects)
{
    // Precondition: Intersections are sorted bottom-most first.
    // It's crucial that intersections are only made between adjacent edges,
    // so reorder the intersections to ensure this if necessary.

    copy_AEL_to_SEL(active_edge_list, sorted_edge_list);
    std::stable_sort(intersects.begin(), intersects.end(),
                     intersect_list_sorter());

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
