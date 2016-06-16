#include "catch.hpp"

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/sorted_edge_list.hpp>

TEST_CASE("sorted edge list -- pop") {
    using namespace mapbox::geometry::wagyu;
    mapbox::geometry::point<std::int64_t> p1 = { 100, 10 };
    mapbox::geometry::point<std::int64_t> p2 = { 100, 10 };
    edge<std::int64_t> e1(p1, p2, polygon_type_subject);
    edge<std::int64_t> e2(p1, p2, polygon_type_subject);
    edge_ptr<std::int64_t> sorted_edge_list;

    sorted_edge_list = &e1;
    e1.next_in_SEL = &e2;

    edge_ptr<std::int64_t> popped;
    CHECK(pop_edge_from_SEL(popped, sorted_edge_list));
    CHECK(&e1 == popped);
    CHECK(sorted_edge_list != popped);
    CHECK(sorted_edge_list == &e2);
};