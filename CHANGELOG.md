# Changelog

## 0.1.0 

- Initial Release of Wagyu

## 0.2.0

- Fixed winding order issues related to [issue #51](https://github.com/mapbox/wagyu/issues/51)

## 0.3.0

- Added quick clip as path for quickly clipping large polygon data specifically to a bounding box.
- Removed linestring code from wagyu, going forward library is only planning on supporting polygon data.
- Removed some dead code paths
- Fixed some bugs associated with multipolygons that are intersecting
- Fixed rare bug where holes were sometimes being considered as new polygons

## 0.4.0

- Completely reworked the way topology correction works. It is not seperated into more discrete steps rather then attempting to process it all in one loop through all points. This has made the code much easier to debug.
- Removed the need to process certain intersections before others in order to gain correct results.
- Updated `poly2_contain_poly1` such that the rare situation where one ring contains all the same points as another ring, it properly returns results in all situations.
- Added several more fields to the `ring` struct so that it now tracks area and size more efficiently. Also added a bounding box to the calculation for each `ring`.

