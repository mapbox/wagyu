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
}
}
}
