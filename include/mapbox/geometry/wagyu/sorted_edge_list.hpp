#pragma once


namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
void add_edge_to_SEL(edge_ptr<T> edge)
{
    //SEL pointers in PEdge are reused to build a list of horizontal edges.
    //However, we don't need to worry about order with horizontal edge processing.
    if( !m_SortedEdges )
    {
        m_SortedEdges = edge;
        edge->PrevInSEL = 0;
        edge->NextInSEL = 0;
    }
    else
    {
        edge->NextInSEL = m_SortedEdges;
        edge->PrevInSEL = 0;
        m_SortedEdges->PrevInSEL = edge;
        m_SortedEdges = edge;
    }
}

}}}
