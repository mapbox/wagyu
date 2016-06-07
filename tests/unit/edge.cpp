#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge.hpp>

TEST_CASE("test edge initialization - same two points")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { 100, 10 };
    mapbox::geometry::point<std::int64_t> p2 = { 100, 10 };
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);

    CHECK(e1.Bot.x == 100);
    CHECK(e1.Bot.y == 10);
    CHECK(e1.Curr.x == 100);
    CHECK(e1.Curr.y == 10);
    CHECK(e1.Top.x == 100);
    CHECK(e1.Top.y == 10);
    CHECK(e1.Dx == HORIZONTAL);
    CHECK(e1.Next == nullptr);
    CHECK(e1.Prev == nullptr);
    CHECK(e1.NextInLML == nullptr);
    CHECK(e1.NextInAEL == nullptr);
    CHECK(e1.PrevInAEL == nullptr);
    CHECK(e1.NextInSEL == nullptr);
    CHECK(e1.PrevInSEL == nullptr);
    CHECK(e1.WindCnt == 0);
    CHECK(e1.WindCnt2 == 0);
    CHECK(e1.WindDelta == 0);
    CHECK(e1.PolyTyp == polygon_type_subject);
    CHECK(e1.Side == edge_left);
}

TEST_CASE("test edge initialization - horizontal segment")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { 10, 10 };
    mapbox::geometry::point<std::int64_t> p2 = { 100, 10 };
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);
    edge<std::int64_t> e2(p2, p1, polygon_type_subject);

    CHECK(e1.Bot.x == 10);
    CHECK(e1.Bot.y == 10);
    CHECK(e1.Curr.x == 10);
    CHECK(e1.Curr.y == 10);
    CHECK(e1.Top.x == 100);
    CHECK(e1.Top.y == 10);
    CHECK(e1.Dx == HORIZONTAL);
    
    CHECK(e2.Bot.x == 100);
    CHECK(e2.Bot.y == 10);
    CHECK(e2.Curr.x == 100);
    CHECK(e2.Curr.y == 10);
    CHECK(e2.Top.x == 10);
    CHECK(e2.Top.y == 10);
    CHECK(e2.Dx == HORIZONTAL);
}

TEST_CASE("test edge initialization - vertical segment")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { 10, 10 };
    mapbox::geometry::point<std::int64_t> p2 = { 10, 100 };
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);
    edge<std::int64_t> e2(p2, p1, polygon_type_subject);

    CHECK(e1.Bot.x == 10);
    CHECK(e1.Bot.y == 100);
    CHECK(e1.Curr.x == 10);
    CHECK(e1.Curr.y == 10);
    CHECK(e1.Top.x == 10);
    CHECK(e1.Top.y == 10);
    CHECK(e1.Dx == 0.0);

    CHECK(e2.Bot.x == 10);
    CHECK(e2.Bot.y == 100);
    CHECK(e2.Curr.x == 10);
    CHECK(e2.Curr.y == 100);
    CHECK(e2.Top.x == 10);
    CHECK(e2.Top.y == 10);
    CHECK(e2.Dx == 0.0);
}
