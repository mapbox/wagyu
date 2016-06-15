#pragma once

namespace mapbox
{
namespace geometry
{
namespace wagyu
{
template <typename T>
void add_edge_to_SEL(edge_ptr<T> edge)
{
    // SEL pointers in PEdge are reused to build a list of horizontal edges.
    // However, we don't need to worry about order with horizontal edge
    // processing.
    if (!m_SortedEdges)
    {
        m_SortedEdges = edge;
        edge->prevInSEL = 0;
        edge->nextInSEL = 0;
    }
    else
    {
        edge->nextInSEL = m_SortedEdges;
        edge->prevInSEL = 0;
        m_SortedEdges->prevInSEL = edge;
        m_SortedEdges = edge;
    }
}
}
}
}
