#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge_util.hpp>

TEST_CASE("test reverse horizontal")
{
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = {0, 5};
    mapbox::geometry::point<std::int64_t> p2 = {5, 5};
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);

    CHECK(e1.bot.x == 0);
    CHECK(e1.bot.y == 5);
    CHECK(e1.curr.x == 0);
    CHECK(e1.curr.y == 5);
    CHECK(e1.top.x == 5);
    CHECK(e1.top.y == 5);

    ReverseHorizontal(e1);

    CHECK(e1.bot.x == 5);
    CHECK(e1.bot.y == 5);
    CHECK(e1.curr.x == 0);
    CHECK(e1.curr.y == 5);
    CHECK(e1.top.x == 0);
    CHECK(e1.top.y == 5);
}

TEST_CASE("edge adding ring - square closed")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square not closed")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - triangle closed")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({10, 5});
    ring.push_back({5, 10});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 3);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 2.0);
    ++itr;
    CHECK(itr->top.x == 10);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->curr.x == 10);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(-1.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 10);
    CHECK(itr->dx == Approx(0.5));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - triangle not closed")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({10, 5});
    ring.push_back({5, 10});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 3);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 2.0);
    ++itr;
    CHECK(itr->top.x == 10);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->curr.x == 10);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(-1.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 10);
    CHECK(itr->dx == Approx(0.5));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square closed - collinear points")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 3});
    ring.push_back({0, 5});
    ring.push_back({3, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 3});
    ring.push_back({5, 0});
    ring.push_back({3, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square not closed - collinear points")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({4, 0});
    ring.push_back({3, 0});
    ring.push_back({2, 0});
    ring.push_back({1, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square closed - repeated points")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({5, 0});
    ring.push_back({0, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square closed - repeated and collinear points")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 0});
    ring.push_back({0, 3});
    ring.push_back({0, 3});
    ring.push_back({0, 5});
    ring.push_back({0, 5});
    ring.push_back({3, 5});
    ring.push_back({3, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 3});
    ring.push_back({5, 3});
    ring.push_back({5, 0});
    ring.push_back({5, 0});
    ring.push_back({3, 0});
    ring.push_back({3, 0});
    ring.push_back({0, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square closed - spikes")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 10});
    ring.push_back({5, 5});
    ring.push_back({10, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - square closed - zigzag")
{
    using namespace mapbox::geometry::wagyu;

    mapbox::geometry::linear_ring<std::int64_t> ring;
    ring.push_back({0, 0});
    ring.push_back({0, 5});
    ring.push_back({5, 5});
    ring.push_back({5, 0});
    ring.push_back({0, 0});
    ring.push_back({5, 0});
    ring.push_back({0, 0});

    std::vector<edge_list<std::int64_t> > all_edges;
    local_minimum_list<std::int64_t> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto & edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == 0.0);
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(HORIZONTAL));
    ++itr;
    CHECK(itr == edges.end());
}
