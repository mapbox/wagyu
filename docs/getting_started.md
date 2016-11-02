## Getting Started


### Configuration 

Wagyu is a header only library but does have a dependency on [Mapbox Geometry](https://github.com/mapbox/geometry.hpp). It is not packaged with the library, but has a similar license and should be included prior to development.

It should be noted that Wagyu requires a compiler that supports at least C++11. You can garuantee that C++11 is used by including the `-std=c++11` flag with most compilers.

### Geometry Operations

Wagyu supports the following geometric operations:

* Union
* Intersection
* Difference
* XOR


