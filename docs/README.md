## Wagyu Documentation

Welcome to the Wagyu documentation, the purpose of this documentation is to explain the concepts
and algorithm used within Wagyu. 

### Origins of Wagyu

Wagyu originated as a fork of the [Angus Johnson Clipper Library](http://www.angusj.com/delphi/clipper.php) and still
shares some of the same code, however, some of the algorithm has been changed. Both libraries still utilize the [Vatti Clipping Algorithm](https://en.wikipedia.org/wiki/Vatti_clipping_algorithm). Wagyu, however, follows this clipping algorithm up with a topology correction algorithm. This is used to guarantee that all geometry created by the Vatti clipping is returned as being Valid and Simple as per the OGC specification. 

### Documentation Map

* Wagyu Algorithm Documentation
    * [Algorithm Overview](overview.md)
    * [Vatti Algorithm](vatti.md)
        * [Vatti Intersections](vatti_intersections.md)
        * [Snap Rounding](snap_rounding.md)
    * Topology Correction
        * [Point Intersections](point_intersections.md)
* Examples and Usage
    * [Getting Started](getting_started.md)
    * [Example Code](example.md)
* Contributing to Wagyu
    * [Building and Testing](building_and_testing.md)
