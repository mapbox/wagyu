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
- Replaced the scanbeam tracking to no longer use a priority queue as a std vector was shown to be slightly faster
- Replaced std list in active bounds list with a std vector
- Fixed bug in bound construtors where `next_edge` was not being properly initialized. 
- Fixed bug in snap rounding where `next_edge` of bounds were not being properly set.
- Added ability for `fixture-tester` to repeatedly test the same test. 

## 0.4.1

- The integer type of input and output can now be different then the integer type used in wagyu's processing
- Added -Wshorten-64-to-32 to warnings during builds

## 0.4.2

- Fixed issue found with -Wconversion through out the code
- Put rounding into place in quick clip when it was previously truncating points while clipping.
- Removed unrequired referencing of children ring pointers in several locations within loops
- Switched to `mason.sh` client script over including entire mason repository
- Deleted default copy constructor on bound structure
- Fixed bug in `get_dx`
- Fixed some includes that were missing in some headers
- Removed some dead code paths and checks that are no longer required

## 0.4.3

- Use `::llround()` instead of `std::llround()` for old libstdc++ compatibility.

## 0.5.0

- Fixed various issues associated with floating point data and comparisions of numbers. This in effect solves some weird intersection bugs and situations where the bounds are not properly sorted. This can result in a variety of different crashes and bad results in the final output in very rare situations.
