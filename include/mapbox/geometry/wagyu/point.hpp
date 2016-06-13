#pragma once

#include <mapbox/geometry/point.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
struct point;

template <typename T>
using point_ptr = point<T> *;

template <typename T>
using const_point_ptr = point<T> * const;

template <typename T>
struct point
{
    using coordinate_type = T;
    std::size_t index;
    T x;
    T y;
    point_ptr<T> next;
    point_ptr<T> prev;
    
    point() :
        index(0),
        x(0), 
        y(0),
        prev(this),
        next(this) {}
    
    point(T x_, T y_) :
        index(0),
        x(x_),
        y(y_),
        next(this),
        prev(this) {}

    point(std::size_t index_,
          mapbox::geometry::point<T> const& pt) :
        index(index_),
        x(pt.x),
        y(pt.y),
        next(this),
        prev(this) {}
    
    point(std::size_t index_,
          mapbox::geometry::point<T> const& pt,
          point_ptr<T> before_this_point) :
        index(index_),
        x(pt.x),
        y(pt.y),
        next(before_this_point),
        prev(before_this_point->prev)
    {
        before_this_point->prev = this;
        prev->next = this;
    }
};

template <typename T>
bool operator==(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator==(mapbox::geometry::point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator==(point<T> const& lhs, mapbox::geometry::point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template <typename T>
bool operator!=(mapbox::geometry::point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

template <typename T>
bool operator!=(point<T> const& lhs, mapbox::geometry::point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

}}}
