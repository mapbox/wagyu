## Intersections

Intersections of points will be found after the vatti processing. Every point will be part of a ring at this point and no ring will overlap another ring. It is the job of the topology correction code to take the existing rings and handle the different intersections to make valid and simple polygons.

### Types of Intersections

The following are the types of intersections that can occur

* Intersection of points on the same ring
* Intersetion of an exterior ring with another exterior ring
* Intersection of an exterior ring with an interior ring
* Intersection of an interior ring with another interior ring

#### Self Intersection

A self intersection is an instance where two points belong to the same ring. Each time this occurs a section of the ring will become a new ring.

Example of a self intersection:

![Simple self Intersection](/images/simple_self_intersection.png)

