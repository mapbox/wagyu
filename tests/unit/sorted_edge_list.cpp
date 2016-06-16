#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

TEST_CASE("sorted edge list -- pop") {
    mapbox::geometry::point<T> p1 = { 100, 10 };
    mapbox::geometry::point<T> p2 = { 100, 10 };
    edge<T> e1(p1, p2, polygon_type_subject);
    edge<T> e2(p1, p2, polygon_type_subject);
    edge_ptr<T> sorted_edge_list;

    sorted_edge_list = &e1;
    e1.next_in_SEL = &e2;

    edge_ptr<T> popped;
    CHECK(pop_edge_from_SEL(popped, sorted_edge_list));
    CHECK(&e1 == popped);
    CHECK(sorted_edge_list != popped);
    CHECK(sorted_edge_list == &e2);
};