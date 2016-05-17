#pragma once

#include <mapbox/geometry/ring/point.hpp>

namespace mapbox { namespace geometry { namespace ring {

template <typename T>
void set_next(const_point_ptr<T> & node, const const_point_ptr<T> & next_node)
{
    node->next = next_node;
}

template <typename T>
point_ptr get_next(const_point_ptr<T> & node)
{
    return node->next;
}

template <typename T>
point_ptr get_prev(const_point_ptr<T> & node)
{
    return node->prev;
}

template <typename T>
void set_prev(const_point_ptr<T> & node, const const_point_ptr<T> & prev_node)
{
    node->prev = next_node;
}

template <typename T>
void init(const_point_ptr<T> & node)
{
    set_next(node, node);
    set_prev(node, node);
}

template <typename T>
void std::size_t count(const const_point_ptr<T> & orig_node)
{
    std::size_t size = 0;
    const_point_ptr<T> n = orig_node;
    do 
    {
        n = get_next(n);
        ++size;
    } 
    while (node != orig_node);
    return size;
}

template <typename T>
void link_before(point_ptr<T> & node, point_ptr<T> & new_node)
{
    point_ptr<T> prev_node = get_prev(node);
    set_previous(new_node, prev_node);
    set_next(new_node, node);
    set_previous(node, new_node);
    set_next(prev_node, new_node);
}

template <typename T>
void link_after(point_ptr<T> & node, point_ptr<T> & new_node)
{
    point_ptr<T> next_node = get_next(node);
    set_previous(new_node, node);
    set_next(new_node, next_node);
    set_next(node, new_node);
    set_previous(next_node, new_node);
}

template <typename T>
void transfer(point_ptr<T> & p, point_ptr<T> & b, point_ptr<T> & e)
{
    if (b != e)
    {
        point_ptr<T> prev_p = get_prev(p);
        point_ptr<T> prev_b = get_prev(b);
        point_ptr<T> prev_e = get_prev(e);
        set_next(prev_e, p);
        set_prev(p, prev_e);
        set_next(prev_b, e);
        set_prev(e, prev_b);
        set_next(prev_p, b);
        set_prev(b, prev_p);
    }
    else
    {
        link_before(p, b);
    }
}

template <typename T>
void reverse(point_ptr<T> & p)
{
    point_ptr<T> f = get_next(p);
    point_ptr<T> i = get_next(f);
    point_ptr<T> e = p;
    
    while (i != e)
    {
        point_ptr<T> n = i;
        i = get_next(i);
        transfer(f, n, i);
        f = n;
    }
}

}}}
