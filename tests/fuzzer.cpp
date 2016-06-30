#include "util/boost_geometry_adapters.hpp"
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

int main() {

    while (1) {
        std::size_t len = std::rand() % 10 + 3;

        for (auto fill_type : { mapbox::geometry::wagyu::fill_type_even_odd,
                                mapbox::geometry::wagyu::fill_type_non_zero,
                                mapbox::geometry::wagyu::fill_type_positive,
                                mapbox::geometry::wagyu::fill_type_negative }) {

            mapbox::geometry::wagyu::clipper<std::int64_t> clipper;
            mapbox::geometry::polygon<std::int64_t> polygon;

            std::size_t num_rings = 1;
            //num_rings += std::rand() % 5;
            // std::clog << "rings: " << num_rings << std::endl;
            // std::clog << "len: " << len << std::endl;
            while (num_rings > 0) {
                mapbox::geometry::linear_ring<std::int64_t> ring;
                for (std::size_t i = 0; i < len; ++i) {
                    std::int64_t x = std::rand() % 4096;
                    std::int64_t y = std::rand() % 4096;

                    ring.push_back({ x, y });
                }
                polygon.emplace_back(ring);
                --num_rings;
            }

            std::clog << ".";
            clipper.add_polygon(polygon, mapbox::geometry::wagyu::polygon_type_subject);
            std::vector<mapbox::geometry::polygon<std::int64_t>> solution;
            clipper.execute(mapbox::geometry::wagyu::clip_type_union, solution, fill_type,
                            mapbox::geometry::wagyu::fill_type_even_odd);

            for (auto const& p : solution) {
                std::string message;
                if (!boost::geometry::is_valid(p, message)) {
                    std::clog << message << std::endl;
                }
            }
        }
    }
}
