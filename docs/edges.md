### Edges

An edge is a line formed between two points on a ring.

Edges are defined in the [edge.hpp](https://github.com/mapbox/wagyu/blob/master/include/mapbox/geometry/wagyu/edge.hpp).

The basic structure of an edge is:

```
struct edge {
    mapbox::geometry::point<T> bot;
    mapbox::geometry::point<T> top;
    double dx;
};
```

Edges are only used in Wagyu to represent the pieces of a [bound](bounds.md). 
