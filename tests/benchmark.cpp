#include "util/boost_geometry_adapters.hpp"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <ostream>
#include <sstream>

#include "clipper.hpp"
#include "rapidjson/writer.h"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

using namespace rapidjson;
using namespace mapbox::geometry::wagyu;
using value_type = std::int64_t;

struct Options {
    clip_type operation = clip_type_intersection;
    fill_type fill = fill_type_even_odd;
    char* subject_file;
    char* clip_file;
    std::size_t iter = 100;
} options;

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

mapbox::geometry::polygon<value_type> parse_file(const char* file_path) {
    // todo safety checks opening files
    FILE* file = fopen(file_path, "r");
    char read_buffer[65536];
    FileReadStream in_stream(file, read_buffer, sizeof(read_buffer));
    Document document;
    document.ParseStream<0, UTF8<>, FileReadStream>(in_stream);

    if (!document.IsArray()) {
        throw std::runtime_error(("Input file (" + std::string(file_path) + ") is not valid json"));
    }
    // todo catch parsing errors
    mapbox::geometry::polygon<value_type> poly;
    for (SizeType i = 0; i < document.Size(); ++i) {
        mapbox::geometry::linear_ring<value_type> lr;

        if (!document[i].IsArray()) {
            throw std::runtime_error("A ring (in " + std::string(file_path) +
                                     ") is not a valid json array");
        }
        for (SizeType j = 0; j < document[i].Size(); ++j) {
            lr.push_back({ document[i][j][0].GetInt(), document[i][j][1].GetInt() });
        }
        poly.emplace_back(lr);
    }
    fclose(file);
    return poly;
}

void polys_to_json(Document& output, std::vector<mapbox::geometry::polygon<value_type>>& solution) {
    output.SetArray();
    Document::AllocatorType& allocator = output.GetAllocator();
    output.Reserve(solution.size(), allocator);

    // Polygons
    for (std::size_t p = 0; p < solution.size(); ++p) {
        output.PushBack(Value().SetArray(), allocator);
        output[p].Reserve(solution[p].size(), allocator);

        // Rings
        for (std::size_t r = 0; r < solution[p].size(); ++r) {
            output[p].PushBack(Value().SetArray(), allocator);
            output[p][r].Reserve(solution[p][r].size(), allocator);

            // Coordinates
            for (auto coord : solution[p][r]) {
                Value cvalue;
                cvalue.SetArray();
                cvalue.PushBack(Value().SetInt(coord.x), allocator);
                cvalue.PushBack(Value().SetInt(coord.y), allocator);
                output[p][r].PushBack(cvalue, allocator);
            }
        }
    }
}

void parse_options(int argc, char* const argv[]) {
    bool skip = false;
    for (int i = 1; i < argc; ++i) {
        // if this argument is already being used
        // as the value for a flag, we skip it
        if (skip) {
            skip = false;
            continue;
        }

        if (strcmp(argv[i], "-t") == 0) {
            std::string type = argv[i + 1];
            if (type.compare("union") == 0) {
                options.operation = clip_type_union;
            } else if (type.compare("intersection") == 0) {
                options.operation = clip_type_intersection;
            } else if (type.compare("difference") == 0) {
                options.operation = clip_type_difference;
            } else if (type.compare("x_or") == 0) {
                options.operation = clip_type_x_or;
            }
            skip = true;
        } else if (strcmp(argv[i], "-f") == 0) {
            std::string type = argv[i + 1];
            if (type.compare("even_odd") == 0) {
                options.fill = fill_type_even_odd;
            } else if (type.compare("non_zero") == 0) {
                options.fill = fill_type_non_zero;
            } else if (type.compare("positive") == 0) {
                options.fill = fill_type_positive;
            } else if (type.compare("negative") == 0) {
                options.fill = fill_type_negative;
            }
            skip = true;
        } else if (strcmp(argv[i], "-i") == 0) {
            std::istringstream ss(argv[i + 1]);
            if (!(ss >> options.iter)) {
                std::clog << "Invalid number " << argv[1] << std::endl;
            }
            skip = true;
        } else {
            // If we didn't catch this argument as a flag or a flag value,
            // set the input files
            if (options.subject_file == NULL) {
                options.subject_file = argv[i];
            } else {
                options.clip_file = argv[i];
            }
        }
    }
}

inline void process_polynode_branch(ClipperLib::PolyNode* polynode,
                                    mapbox::geometry::multi_polygon<std::int64_t>& mp) {
    mapbox::geometry::polygon<std::int64_t> polygon;
    polygon.push_back(std::move(polynode->Contour));
    if (polygon.back().size() > 2) // Throw out invalid polygons
    {
        if (polygon.back().back() != polygon.back().front()) {
            polygon.back().push_back(polygon.back().front());
        }
        double outer_area = ClipperLib::Area(polygon.back());
        if (outer_area > 0) {
            std::reverse(polygon.back().begin(), polygon.back().end());
        }

        // children of exterior ring are always interior rings
        for (auto* ring : polynode->Childs) {
            if (ring->Contour.size() < 3) {
                continue; // Throw out invalid holes
            }
            double inner_area = ClipperLib::Area(ring->Contour);

            if (inner_area < 0) {
                std::reverse(ring->Contour.begin(), ring->Contour.end());
            }
            polygon.push_back(std::move(ring->Contour));
            if (polygon.back().back() != polygon.back().front()) {
                polygon.back().push_back(polygon.back().front());
            }
        }
        mp.push_back(std::move(polygon));
    }
    for (auto* ring : polynode->Childs) {
        for (auto* sub_ring : ring->Childs) {
            process_polynode_branch(sub_ring, mp);
        }
    }
}

inline ClipperLib::PolyFillType get_angus_fill_type(fill_type type) {
    switch (type) {
    case fill_type_even_odd:
        return ClipperLib::pftEvenOdd;
    case fill_type_non_zero:
        return ClipperLib::pftNonZero;
    case fill_type_positive:
        return ClipperLib::pftPositive;
    case fill_type_negative:
        return ClipperLib::pftNegative;
    }
}

inline ClipperLib::ClipType get_angus_clip_type(clip_type type) {
    switch (type) {
    case clip_type_intersection:
        return ClipperLib::ctIntersection;
    case clip_type_union:
        return ClipperLib::ctUnion;
    case clip_type_difference:
        return ClipperLib::ctDifference;
    case clip_type_x_or:
        return ClipperLib::ctXor;
    }
}

int main(int argc, char* const argv[]) {
    if (argc < 3) {
        std::cout << "Error: too few parameters\n" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  ./benchmark ./path/to/subject.json ./path/to/object.json\n" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -t     type of operation (default: union)\n" << std::endl;
        std::cout << "  -f     fill_type (default: even_odd)\n" << std::endl;
        std::cout << "  -i     number of iterations for test\n" << std::endl;
        return -1;
    }
    parse_options(argc, argv);

    auto poly_subject = parse_file(options.subject_file);
    auto poly_clip = parse_file(options.clip_file);

    using double_seconds = std::chrono::duration<double, std::chrono::seconds::period>;

    double time_wagyu;
    double time_angus;
    bool wagyu_valid = true;
    bool angus_valid = true;
    {
        wagyu<value_type> clipper;
        clipper.add_polygon(poly_subject, polygon_type_subject);
        clipper.add_polygon(poly_clip, polygon_type_clip);
        mapbox::geometry::multi_polygon<value_type> solution;
        clipper.execute(options.operation, solution, options.fill, fill_type_even_odd);

        for (auto const& p : solution) {
            std::string message;
            if (!boost::geometry::is_valid(p, message)) {
                wagyu_valid = false;
            }
        }
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();

        for (std::size_t i = 0; i < options.iter; ++i) {
            wagyu<value_type> clipper;
            clipper.add_polygon(poly_subject, polygon_type_subject);
            clipper.add_polygon(poly_clip, polygon_type_clip);
            mapbox::geometry::multi_polygon<value_type> solution;
            clipper.execute(options.operation, solution, options.fill, fill_type_even_odd);
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_wagyu = double_seconds(t2 - t1).count();
    }

    ClipperLib::PolyFillType angus_fill_type = get_angus_fill_type(options.fill);
    ClipperLib::ClipType angus_clip_type = get_angus_clip_type(options.operation);
    {
        ClipperLib::Clipper clipper;
        clipper.StrictlySimple(true);
        for (auto& r : poly_subject) {
            // ClipperLib::CleanPolygon(r, 1.415);
            clipper.AddPath(r, ClipperLib::ptSubject, true);
        }
        for (auto& r : poly_clip) {
            clipper.AddPath(r, ClipperLib::ptClip, true);
        }
        ClipperLib::PolyTree polygons;
        clipper.Execute(angus_clip_type, polygons, angus_fill_type, ClipperLib::pftEvenOdd);
        clipper.Clear();
        mapbox::geometry::multi_polygon<value_type> solution;

        for (auto* polynode : polygons.Childs) {
            process_polynode_branch(polynode, solution);
        }

        for (auto const& p : solution) {
            std::string message;
            if (!boost::geometry::is_valid(p, message)) {
                angus_valid = false;
            }
        }
    }

    {
        auto t1 = std::chrono::high_resolution_clock::now();

        for (std::size_t i = 0; i < options.iter; ++i) {
            ClipperLib::Clipper clipper;
            clipper.StrictlySimple(true);
            for (auto& r : poly_subject) {
                // ClipperLib::CleanPolygon(r, 1.415);
                clipper.AddPath(r, ClipperLib::ptSubject, true);
            }
            for (auto& r : poly_clip) {
                clipper.AddPath(r, ClipperLib::ptClip, true);
            }
            ClipperLib::PolyTree polygons;
            clipper.Execute(angus_clip_type, polygons, angus_fill_type, ClipperLib::pftEvenOdd);
            clipper.Clear();
            mapbox::geometry::multi_polygon<value_type> solution;

            for (auto* polynode : polygons.Childs) {
                process_polynode_branch(polynode, solution);
            }
        }
        auto t2 = std::chrono::high_resolution_clock::now();
        time_angus = double_seconds(t2 - t1).count();
    }
    if (wagyu_valid) {
        std::clog << "\033[1;32m";
    } else {
        std::clog << "\033[0;31m";
    }
    std::clog << time_wagyu << "\033[0m,  ";
    if (angus_valid) {
        std::clog << "\033[1;32m";
    } else {
        std::clog << "\033[0;31m";
    }
    std::clog << time_angus << "\033[0m  - ";
    double factor = time_wagyu / time_angus;
    if (factor < 1.0) {
        std::clog << "\033[1;34m";
    } else {
        std::clog << "\033[0;36m";
    }
    std::clog << factor << "\033[0m" << std::endl;
    if (factor > 1.0) {
        return -1;
    }
}
