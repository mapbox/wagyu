#include "catch.hpp"

#include <mapbox/geometry/wagyu/wagyu.hpp>

TEST_CASE("test returns zero with no data provided - int64")
{
    mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
    auto bounds = clipper.get_bounds();
    CHECK(bounds.left == 0);
    CHECK(bounds.top == 0);
    CHECK(bounds.right == 0);
    CHECK(bounds.bottom == 0);
}
