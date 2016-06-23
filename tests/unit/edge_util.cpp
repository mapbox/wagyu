#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge_util.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("test reverse horizontal") {
    mapbox::geometry::point<T> p1 = { 0, 5 };
    mapbox::geometry::point<T> p2 = { 5, 5 };
    edge<T> e1(p1, p2, polygon_type_subject);

    CHECK(e1.bot.x == 0);
    CHECK(e1.bot.y == 5);
    CHECK(e1.curr.x == 0);
    CHECK(e1.curr.y == 5);
    CHECK(e1.top.x == 5);
    CHECK(e1.top.y == 5);

    reverse_horizontal(e1);

    CHECK(e1.bot.x == 5);
    CHECK(e1.bot.y == 5);
    CHECK(e1.curr.x == 0);
    CHECK(e1.curr.y == 5);
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("test is minima") {
    mapbox::geometry::point<T> p1 = { 0, 5 };
    mapbox::geometry::point<T> p2 = { 5, 5 };
    edge<T> e1(p1, p2, polygon_type_subject);
    edge<T> e2(p1, p2, polygon_type_subject);
    edge<T> e3(p1, p2, polygon_type_subject);

    e1.prev = &e3;
    e1.next = &e2;
    e2.prev = &e1;
    e2.next = &e3;
    e3.prev = &e2;
    e3.next = &e1;
    e1.next_in_LML = &e3;
    e2.next_in_LML = &e3;
    e3.next_in_LML = &e3;

    // neither prev/next next_in_LML points back at e2
    CHECK(is_minima(&e2) == true);
    // both prev/next next_in_LML points back to e3
    CHECK(is_minima(&e3) == false);
}

TEST_CASE("edge adding ring - square not closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding ring - triangle closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 10 });
    ring.push_back({ 0, 0 });

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 3);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(2.0));
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

TEST_CASE("edge adding ring - triangle not closed") {
    mapbox::geometry::linear_ring<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 10, 5 });
    ring.push_back({ 5, 10 });

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 3);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 10);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(2.0));
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
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

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_linear_ring(ring, all_edges, minima_list, p_type));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 4);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 0);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 0);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr == edges.end());
}

TEST_CASE("edge adding linestring") {
    mapbox::geometry::line_string<T> ring;
    ring.push_back({ 0, 0 });
    ring.push_back({ 0, 5 });
    ring.push_back({ 5, 5 });
    ring.push_back({ 5, 0 });

    std::vector<edge_list<T>> all_edges;
    local_minimum_list<T> minima_list;
    polygon_type p_type = polygon_type_subject;

    CHECK(add_line_string(ring, all_edges, minima_list));

    auto& edges = all_edges.back();
    REQUIRE(edges.size() == 3);
    auto itr = edges.begin();
    CHECK(itr->top.x == 0);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 0);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 5);
    CHECK(itr->bot.x == 0);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 0);
    CHECK(itr->curr.y == 5);
    CHECK(std::isinf(itr->dx));
    ++itr;
    CHECK(itr->top.x == 5);
    CHECK(itr->top.y == 0);
    CHECK(itr->bot.x == 5);
    CHECK(itr->bot.y == 5);
    CHECK(itr->curr.x == 5);
    CHECK(itr->curr.y == 5);
    CHECK(itr->dx == Approx(0.0));
    ++itr;
    CHECK(itr == edges.end());
}
