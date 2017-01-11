#include "catch.hpp"

#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/build_local_minima_list.hpp>
#include <mapbox/geometry/wagyu/local_minimum_util.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("test reverse horizontal") {
    mapbox::geometry::point<T> p1 = { 0, 5 };
    mapbox::geometry::point<T> p2 = { 5, 5 };
    edge<T> e1(p1, p2);

    CHECK(e1.bot.x == 0);
    CHECK(e1.bot.y == 5);
    CHECK(e1.top.x == 5);
    CHECK(e1.top.y == 5);

    reverse_horizontal(e1);

    CHECK(e1.bot.x == 5);
    CHECK(e1.bot.y == 5);
    CHECK(e1.top.x == 0);
    CHECK(e1.top.y == 5);
}

TEST_CASE("edge adding ring - square closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square not closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - triangle closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 10 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.right_bound.edges;
    REQUIRE(edges.size() == 2);
    auto itr = edges.begin();
    CHECK(itr->top.x == 10);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->dx == Approx(-1.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(2.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.left_bound.edges;
    REQUIRE(edges_r.size() == 1);
    itr = edges_r.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->dx == Approx(0.5));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - triangle not closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 10 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.right_bound.edges;
    REQUIRE(edges.size() == 2);
    auto itr = edges.begin();
    CHECK(itr->top.x == 10);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->dx == Approx(-1.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(2.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.left_bound.edges;
    REQUIRE(edges_r.size() == 1);
    itr = edges_r.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 10);
    CHECK(itr->dx == Approx(0.5));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square closed - collinear points") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 3 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 3, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 3 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 3, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square not closed - collinear points") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 4, 0 });
    ring.push_back({ 3, 0 });
    ring.push_back({ 2, 0 });
    ring.push_back({ 1, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square closed - repeated points") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square closed - repeated and collinear points") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 3 });
    ring.push_back({ 0, 3 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 3, 5 });
    ring.push_back({ 3, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 3 });
    ring.push_back({ 5, 3 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 3, 0 });
    ring.push_back({ 3, 0 });
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square closed - spikes") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 10 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding ring - square closed - zigzag") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });
    ring.push_back({ 5, 0 });
    ring.push_back({ 0, 0 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 1);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 3);
    itr = edges_r.begin();
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges_r.end());
}

TEST_CASE("edge adding - odd shape") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 65542, 44380 }); // A
    ring.push_back({ 65542, 45062 }); // B
    ring.push_back({ 64947, 45062 }); // C
    ring.push_back({ 64832, 44579 }); // D
    ring.push_back({ 65176, 44820 }); // E
    ring.push_back({ 65542, 44380 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 2);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 64832); // D
    CHECK(itr->top.y == 44579); // D
    CHECK(itr->bot.x == 64947); // C
    CHECK(itr->bot.y == 45062); // C
    CHECK(itr->dx == Approx(0.2380952381));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 2);
    itr = edges_r.begin();
    CHECK(itr->top.x == 65542); // B
    CHECK(itr->top.y == 45062); // B
    CHECK(itr->bot.x == 64947); // C
    CHECK(itr->bot.y == 45062); // C
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 65542); // A
    CHECK(itr->top.y == 44380); // A
    CHECK(itr->bot.x == 65542); // B
    CHECK(itr->bot.y == 45062); // B
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges_r.end());

    auto& lm2 = minima_list.back();
    auto& edges2 = lm2.left_bound.edges;
    REQUIRE(edges2.size() == 1);
    itr = edges2.begin();
    CHECK(itr->top.x == 64832); // D
    CHECK(itr->top.y == 44579); // D
    CHECK(itr->bot.x == 65176); // E
    CHECK(itr->bot.y == 44820); // E
    CHECK(itr->dx == Approx(1.4273858921));
    ++itr;
    CHECK(itr == edges2.end());
    auto& edges2_r = lm2.right_bound.edges;
    REQUIRE(edges2_r.size() == 1);
    itr = edges2_r.begin();
    CHECK(itr->top.x == 65542); // A
    CHECK(itr->top.y == 44380); // A
    CHECK(itr->bot.x == 65176); // E
    CHECK(itr->bot.y == 44820); // E
    CHECK(itr->dx == Approx(-0.8318181818));
    ++itr;
    CHECK(itr == edges2_r.end());
}

TEST_CASE("edge adding insane set of lines") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 65542, 44237 });
    ring.push_back({ 65542, 44505 });
    ring.push_back({ 65542, 44461 });
    ring.push_back({ 65542, 44718 });
    ring.push_back({ 65542, 44766 });
    ring.push_back({ 65542, 44754 });
    ring.push_back({ 65542, 44901 });
    ring.push_back({ 65542, 45003 });
    ring.push_back({ 65542, 45062 }); // B
    ring.push_back({ 64989, 45062 });
    ring.push_back({ 64844, 45062 });
    ring.push_back({ 65032, 45062 });
    ring.push_back({ 65101, 45062 });
    ring.push_back({ 64726, 45062 });
    ring.push_back({ 64457, 45062 });
    ring.push_back({ 63901, 45062 });
    ring.push_back({ 63864, 45062 });
    ring.push_back({ 64336, 45062 });
    ring.push_back({ 64947, 45062 }); // C
    ring.push_back({ 64832, 44579 }); // D
    ring.push_back({ 65176, 44820 }); // E
    ring.push_back({ 65542, 44380 }); // A
    ring.push_back({ 65542, 43911 });
    ring.push_back({ 65542, 43794 });
    ring.push_back({ 65542, 43997 });
    ring.push_back({ 65542, 44007 });
    ring.push_back({ 65542, 44237 });

    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, minima_list, p_type));

    REQUIRE(minima_list.size() == 2);
    auto& lm = minima_list.front();
    auto& edges = lm.left_bound.edges;
    REQUIRE(edges.size() == 1);
    auto itr = edges.begin();
    CHECK(itr->top.x == 64832); // D
    CHECK(itr->top.y == 44579); // D
    CHECK(itr->bot.x == 64947); // C
    CHECK(itr->bot.y == 45062); // C
    CHECK(itr->dx == Approx(0.2380952381));
    ++itr;
    CHECK(itr == edges.end());
    auto& edges_r = lm.right_bound.edges;
    REQUIRE(edges_r.size() == 2);
    itr = edges_r.begin();
    CHECK(itr->top.x == 65542); // B
    CHECK(itr->top.y == 45062); // B
    CHECK(itr->bot.x == 64947); // C
    CHECK(itr->bot.y == 45062); // C
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 65542); // A
    CHECK(itr->top.y == 44380); // A
    CHECK(itr->bot.x == 65542); // B
    CHECK(itr->bot.y == 45062); // B
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges_r.end());

    auto& lm2 = minima_list.back();
    auto& edges2 = lm2.left_bound.edges;
    REQUIRE(edges2.size() == 1);
    itr = edges2.begin();
    CHECK(itr->top.x == 64832); // D
    CHECK(itr->top.y == 44579); // D
    CHECK(itr->bot.x == 65176); // E
    CHECK(itr->bot.y == 44820); // E
    CHECK(itr->dx == Approx(1.4273858921));
    ++itr;
    CHECK(itr == edges2.end());
    auto& edges2_r = lm2.right_bound.edges;
    REQUIRE(edges2_r.size() == 1);
    itr = edges2_r.begin();
    CHECK(itr->top.x == 65542); // A
    CHECK(itr->top.y == 44380); // A
    CHECK(itr->bot.x == 65176); // E
    CHECK(itr->bot.y == 44820); // E
    CHECK(itr->dx == Approx(-0.8318181818));
    ++itr;
    CHECK(itr == edges2_r.end());
}
