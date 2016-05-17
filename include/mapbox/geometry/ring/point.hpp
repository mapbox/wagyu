#pragma once

namespace mapbox { namespace geometry { namespace circular_ring {

// fwd declare
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
    point()
        : x(), y()
    {}
    point(T x_, T y_)
        : x(x_), y(y_)
    {}
    T x;
    T y;
    point_ptr<T> prev;
    point_ptr<T> next;
};

template <typename T>
bool operator==(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

template <typename T>
bool operator!=(point<T> const& lhs, point<T> const& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

}}}
