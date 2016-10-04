#include "util/boost_geometry_adapters.hpp"
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/multi_polygon.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <time.h>
#include <vector>

void log_ring(mapbox::geometry::polygon<std::int64_t> const& p) {
    bool first = true;
    std::clog << "[";
    for (auto const& r : p) {
        if (first) {
            std::clog << "[";
            first = false;
        } else {
            std::clog << ",[";
        }
        bool first2 = true;
        for (auto const& pt : r) {
            if (first2) {
                std::clog << "[";
                first2 = false;
            } else {
                std::clog << ",[";
            }
            std::clog << pt.x << "," << pt.y << "]";
        }
        std::clog << "]";
    }
    std::clog << "]" << std::endl;
}

void log_ring(mapbox::geometry::multi_polygon<std::int64_t> const& mp) {
    bool first_p = true;
    std::clog << "[";
    for (auto const& p : mp) {
        bool first = true;
        if (first_p) {
            std::clog << "[";
            first_p = false;
        } else {
            std::clog << ",[";
        }
        for (auto const& r : p) {
            if (first) {
                std::clog << "[";
                first = false;
            } else {
                std::clog << ",[";
            }
            bool first2 = true;
            for (auto const& pt : r) {
                if (first2) {
                    std::clog << "[";
                    first2 = false;
                } else {
                    std::clog << ",[";
                }
                std::clog << pt.x << "," << pt.y << "]";
            }
            std::clog << "]";
        }
        std::clog << "]";
    }
    std::clog << "]" << std::endl;
}

void check_cross_ring(std::vector<mapbox::geometry::polygon<std::int64_t>>& solution) {
    bool found_error = false;

    std::vector<boost::geometry::model::segment<mapbox::geometry::point<std::int64_t>>> segments;
    for (size_t i = 0; i < solution.size(); i++) {
        for (size_t j = 0; j < solution[i].size(); j++) {
            for (size_t k = 0; k < solution[i][j].size(); k++) {
                segments.push_back(
                    boost::geometry::model::segment<mapbox::geometry::point<std::int64_t>>(
                        solution[i][j][k], solution[i][j][(k + 1) % solution[i][j].size()]));
            }
        }
    }

    for (size_t i = 0; i < segments.size(); i++) {
        for (size_t j = i + 1; j < segments.size(); j++) {
            std::vector<mapbox::geometry::point<std::int64_t>> xings;
            boost::geometry::intersection(segments[i], segments[j], xings);

            if (xings.size() > 0) {
                for (size_t k = 0; k < xings.size(); k++) {
                    if (xings[k] != segments[i].first && xings[k] != segments[i].second &&
                        xings[k] != segments[j].first && xings[k] != segments[j].second) {
                        found_error = true;

                        std::clog << "Intersection between rings: ";
                        std::clog << xings[k].x << "," << xings[k].y;
                        std::clog << " in ";

                        std::clog << segments[i].first.x << "," << segments[i].first.y << " to ";
                        std::clog << segments[i].second.x << "," << segments[i].second.y << " and ";
                        std::clog << segments[j].first.x << "," << segments[j].first.y << " to ";
                        std::clog << segments[j].second.x << "," << segments[j].second.y << "\n";
                    }
                }
            }
        }
    }

    if (found_error) {
        for (size_t i = 0; i < solution.size(); i++) {
            log_ring(solution[i]);
        }
        exit(EXIT_FAILURE);
    }
}

void create_test(mapbox::geometry::polygon<std::int64_t> const& polygon,
                 unsigned seed,
                 size_t iteration) {
    std::string fname = "tests/geometry-test-data/input-polyjson/fuzzer-" + std::to_string(seed) +
                        "-" + std::to_string(iteration) + ".json";
    std::clog << "Creating " << fname << "\n";
    std::ofstream out;
    out.open(fname);

    out << "[";

    for (size_t i = 0; i < polygon.size(); i++) {
        if (i != 0) {
            out << ",";
        }

        out << "[";

        for (size_t j = 0; j < polygon[i].size(); j++) {
            if (j != 0) {
                out << ",";
            }

            out << "[" << polygon[i][j].x << "," << polygon[i][j].y << "]";
        }

        out << "]";
    }

    out << "]";

    out.close();
}

void print_clip_type(mapbox::geometry::wagyu::clip_type ct) {
    switch (ct) {
    default:
    case mapbox::geometry::wagyu::clip_type_union:
        std::clog << "Union Clip Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::clip_type_intersection:
        std::clog << "Intersection Clip Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::clip_type_difference:
        std::clog << "Difference Clip Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::clip_type_x_or:
        std::clog << "X OR Clip Type" << std::endl;
        break;
    }
}

void print_fill_type(mapbox::geometry::wagyu::fill_type ft) {
    switch (ft) {
    default:
    case mapbox::geometry::wagyu::fill_type_even_odd:
        std::clog << "Even Odd Fill Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::fill_type_non_zero:
        std::clog << "Non Zero Fill Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::fill_type_positive:
        std::clog << "Positive Fill Type" << std::endl;
        break;
    case mapbox::geometry::wagyu::fill_type_negative:
        std::clog << "Negative Fill Type" << std::endl;
        break;
    }
}

int main() {
    unsigned seed = time(0);
    srand(seed);

    for (size_t iteration = 0;; iteration++) {
        std::size_t len = std::rand() % 50 + 3;

        for (auto clip_type : { mapbox::geometry::wagyu::clip_type_union,
                                mapbox::geometry::wagyu::clip_type_intersection,
                                mapbox::geometry::wagyu::clip_type_difference,
                                mapbox::geometry::wagyu::clip_type_x_or }) {
            for (auto fill_type : { mapbox::geometry::wagyu::fill_type_even_odd,
                                    mapbox::geometry::wagyu::fill_type_non_zero,
                                    mapbox::geometry::wagyu::fill_type_positive,
                                    mapbox::geometry::wagyu::fill_type_negative }) {

                mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
                mapbox::geometry::polygon<std::int64_t> polygon;

                std::size_t num_rings = 1;
                // num_rings += std::rand() % 5;
                // std::clog << "rings: " << num_rings << std::endl;
                // std::clog << "len: " << len << std::endl;
                while (num_rings > 0) {
                    mapbox::geometry::linear_ring<std::int64_t> ring;
                    for (std::size_t i = 0; i < len; ++i) {
                        std::int64_t x = std::rand() % 50;
                        std::int64_t y = std::rand() % 50;

                        ring.push_back({ x, y });
                    }
                    polygon.emplace_back(ring);
                    --num_rings;
                }

                std::clog << ".";
                clipper.add_polygon(polygon, mapbox::geometry::wagyu::polygon_type_subject);
                mapbox::geometry::multi_polygon<std::int64_t> solution;
                try {
                    clipper.execute(clip_type, solution, fill_type,
                                    mapbox::geometry::wagyu::fill_type_even_odd);
                } catch (std::exception const& ex) {
                    create_test(polygon, seed, iteration);
                    std::clog << std::endl;
                    std::clog << ex.what() << std::endl;
                    return -1;
                }

                for (auto const& p : solution) {
                    std::string message;
                    if (!boost::geometry::is_valid(p, message)) {
                        std::clog << std::endl;
                        std::clog << message << std::endl;
                        print_clip_type(clip_type);
                        print_fill_type(fill_type);
                        log_ring(p);
                        create_test(polygon, seed, iteration);
                        return -1;
                    }
                }
                std::string message;
                if (!boost::geometry::is_valid(solution, message)) {
                    std::clog << std::endl;
                    std::clog << "Multipolygon failure case:" << std::endl;
                    std::clog << message << std::endl;
                    print_clip_type(clip_type);
                    print_fill_type(fill_type);
                    log_ring(solution);
                    create_test(polygon, seed, iteration);
                    return -1;
                }

                check_cross_ring(solution);
            }
        }
    }
}
