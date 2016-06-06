#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge_util.hpp>

TEST_CASE("test reverse horizontal")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { 0, 5 };
    mapbox::geometry::point<std::int64_t> p2 = { 5, 5 };
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);

    CHECK(e1.Bot.x == 0);
    CHECK(e1.Bot.y == 5);
    CHECK(e1.Curr.x == 0);
    CHECK(e1.Curr.y == 5);
    CHECK(e1.Top.x == 5);
    CHECK(e1.Top.y == 5);

    ReverseHorizontal(e1);
    
    CHECK(e1.Bot.x == 5);
    CHECK(e1.Bot.y == 5);
    CHECK(e1.Curr.x == 0);
    CHECK(e1.Curr.y == 5);
    CHECK(e1.Top.x == 0);
    CHECK(e1.Top.y == 5);
}
