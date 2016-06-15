#pragma once

namespace mapbox
{
namespace geometry
{
namespace wagyu
{
template <typename T>
void add_edge_to_SEL(edge_ptr<T> edge, edge_ptr<T> & sorted_edges_list)
{
    // SEL pointers in PEdge are reused to build a list of horizontal edges.
    // However, we don't need to worry about order with horizontal edge
    // processing.
    if (!sorted_edges_list)
    {
        sorted_edges_list = edge;
        edge->prev_in_SEL = nullptr;
        edge->next_in_SEL = nullptr;
    }
    else
    {
        edge->next_in_SEL = sorted_edges_list;
        edge->prev_in_SEL = nullptr;
        sorted_edges_list->prev_in_SEL = edge;
        sorted_edges_list = edge;
    }
}

template <typename T>
void swap_positions_in_SEL(edge_ptr<T> edge1,
                           edge_ptr<T> edge2,
                           edge_ptr<T> & sorted_edge_list)
{
    using value_type = T;

    if (!(edge1->next_in_SEL) && !(edge1->prev_in_SEL))
    {
        return;
    }
    if (!(edge2->next_in_SEL) && !(edge2->prev_in_SEL))
    {
        return;
    }

    if (edge1->next_in_SEL == edge2)
    {
        edge_ptr<value_type> next = edge2->next_in_SEL;
        if (next)
        {
            next->prev_in_SEL = edge1;
        }
        edge_ptr<value_type> prev = edge1->prev_in_SEL;
        if (prev)
        {
            prev->next_in_SEL = edge2;
        }
        edge2->prev_in_SEL = prev;
        edge2->next_in_SEL = edge1;
        edge1->prev_in_SEL = edge2;
        edge1->next_in_SEL = next;
    }
    else if (edge2->next_in_SEL == edge1)
    {
        edge_ptr<value_type> next = edge1->next_in_SEL;
        if (next)
        {
            next->prev_in_SEL = edge2;
        }
        edge_ptr<value_type> prev = edge2->prev_in_SEL;
        if (prev)
        {
            prev->next_in_SEL = edge1;
        }
        edge1->prev_in_SEL = prev;
        edge1->next_in_SEL = edge2;
        edge2->prev_in_SEL = edge1;
        edge2->next_in_SEL = next;
    }
    else
    {
        edge_ptr<value_type> next = edge1->next_in_SEL;
        edge_ptr<value_type> prev = edge1->prev_in_SEL;
        edge1->next_in_SEL = edge2->next_in_SEL;
        if (edge1->next_in_SEL)
        {
            edge1->next_in_SEL->prev_in_SEL = edge1;
        }
        edge1->prev_in_SEL = edge2->prev_in_SEL;
        if (edge1->prev_in_SEL)
        {
            edge1->prev_in_SEL->next_in_SEL = edge1;
        }
        edge2->next_in_SEL = next;
        if (edge2->next_in_SEL)
        {
            edge2->next_in_SEL->prev_in_SEL = edge2;
        }
        edge2->prev_in_SEL = prev;
        if (edge2->prev_in_SEL)
        {
            edge2->prev_in_SEL->next_in_SEL = edge2;
        }
    }

    if (!edge1->prev_in_SEL)
    {
        sorted_edge_list = edge1;
    }
    else if (!edge2->prev_in_SEL)
    {
        sorted_edge_list = edge2;
    }
}

template <typename T>
void copy_AEL_to_SEL(const_edge_ptr<T> active_edge_list,
                     edge_ptr<T> & sorted_edge_list)
{
    edge_ptr<T> e = active_edge_list;
    sorted_edge_list = e;
    while (e) {
        e->prev_in_SEL = e->prev_in_AEL;
        e->next_in_SEL = e->next_in_AEL;
        e = e->next_in_AEL;
    }
}
}
}
}
