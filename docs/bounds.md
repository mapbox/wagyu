### Bounds

A bound is a series of [edges](edges.md), that start at a [local mimimum and end at a local maximum](local_min_and_max.md). The edges of a bound are ordered from starting at the local miminum and ending at the local maximum. 

Bounds are important in the vatti algorithm because they share a common set of data, such as a winding delta and winding count. Bounds are not used outside of the vatti algorithm in other steps such as topology correction.

The basic structure of a bound is defined in [bound.hpp](https://github.com/mapbox/wagyu/blob/master/include/mapbox/geometry/wagyu/bound.hpp). A simplified bound structure is provided below, please note not all its fields are shown:

```
struct bound {
    edge_list<T> edges; // A list or vector of edges
    ring_ptr<T> ring; // The current ring
    bound_ptr<T> maximum_bound; // the bound who's maximum connects with this bound
    std::int32_t winding_count; // The current winding count of the bound
    std::int8_t winding_delta;   // 1 or -1 depending on winding direction
    polygon_type poly_type;
}
```


