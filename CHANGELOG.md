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
