#include "util/boost_geometry_adapters.hpp"
#include <cstdio>
#include <iostream>
#include <mapbox/geometry/polygon.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <ostream>

#include "rapidjson/writer.h"
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>

using namespace rapidjson;
using namespace mapbox::geometry::wagyu;
using value_type = std::int64_t;

struct Options {
    std::size_t iterations = 1;
    clip_type operation = clip_type_union;
    fill_type fill = fill_type_even_odd;
    char* subject_file = nullptr;
    char* clip_file = nullptr;
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
    output.Reserve(static_cast<unsigned>(solution.size()), allocator);

    // Polygons
    for (unsigned p = 0; p < solution.size(); ++p) {
        output.PushBack(Value().SetArray(), allocator);
        output[p].Reserve(static_cast<unsigned>(solution[p].size()), allocator);

        // Rings
        for (unsigned r = 0; r < solution[p].size(); ++r) {
            output[p].PushBack(Value().SetArray(), allocator);
            output[p][r].Reserve(static_cast<unsigned>(solution[p][r].size()), allocator);

            // Coordinates
            for (auto coord : solution[p][r]) {
                Value cvalue;
                cvalue.SetArray();
                cvalue.PushBack(Value().SetInt(static_cast<int>(coord.x)), allocator);
                cvalue.PushBack(Value().SetInt(static_cast<int>(coord.y)), allocator);
                output[p][r].PushBack(cvalue, allocator);
            }
        }
    }
}

void parse_options(int argc, char* const argv[]) {
    for (int i = 1; i < argc; ++i) {

        if (strcmp(argv[i], "-t") == 0) {
            std::string type = argv[++i];
            if (type.compare("union") == 0) {
                options.operation = clip_type_union;
            } else if (type.compare("intersection") == 0) {
                options.operation = clip_type_intersection;
            } else if (type.compare("difference") == 0) {
                options.operation = clip_type_difference;
            } else if (type.compare("x_or") == 0) {
                options.operation = clip_type_x_or;
            }
        } else if (strcmp(argv[i], "-f") == 0) {
            std::string type = argv[++i];
            if (type.compare("even_odd") == 0) {
                options.fill = fill_type_even_odd;
            } else if (type.compare("non_zero") == 0) {
                options.fill = fill_type_non_zero;
            } else if (type.compare("positive") == 0) {
                options.fill = fill_type_positive;
            } else if (type.compare("negative") == 0) {
                options.fill = fill_type_negative;
            }
        } else if (strcmp(argv[i], "-i") == 0) {
            std::string iters = argv[++i];
            options.iterations = static_cast<std::size_t>(std::stoul(iters));
        } else {
            // If we didn't catch this argument as a flag or a flag value,
            // set the input files
            if (options.subject_file == nullptr) {
                options.subject_file = argv[i];
            } else {
                options.clip_file = argv[i];
            }
        }
    }
}

int main(int argc, char* const argv[]) {
    if (argc < 3) {
        std::cout << "Error: too few parameters\n" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  ./fixture-test ./path/to/subject.json ./path/to/object.json\n" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -t     type of operation (default: union)\n" << std::endl;
        std::cout << "  -f     fill_type (default: even_odd)\n" << std::endl;
        return -1;
    }
    parse_options(argc, argv);
    auto poly_subject = parse_file(options.subject_file);
    mapbox::geometry::polygon<value_type> poly_clip;
    mapbox::geometry::multi_polygon<value_type> solution;
    if (options.clip_file != nullptr) {
        poly_clip = parse_file(options.clip_file);
    }
    while (options.iterations > 0) {
        options.iterations--;
        solution.clear();

        wagyu<value_type> clipper;
        clipper.add_polygon(poly_subject, polygon_type_subject);
        if (!poly_clip.empty()) {
            clipper.add_polygon(poly_clip, polygon_type_clip);
        }
        clipper.execute(options.operation, solution, options.fill, fill_type_even_odd);
    }

    Document output;
    polys_to_json(output, solution);
    for (auto const& p : solution) {
        std::string message;
        if (!boost::geometry::is_valid(p, message)) {
            std::clog << std::endl;
            std::clog << "Error: geometry not valid" << std::endl;
            std::clog << message << std::endl;
            log_ring(p);
            return -1;
        }
    }
    /*
     * uncomment once https://svn.boost.org/trac/boost/ticket/12503 is resolved
    std::string message;
    if (!boost::geometry::is_valid(solution, message)) {
        std::clog << std::endl;
        std::clog << "Error: multi geometry not valid" << std::endl;
        std::clog << message << std::endl;
        log_ring(solution);
        return -1;
    }
    */
    char write_buffer[65536];
    FileWriteStream out_stream(stdout, write_buffer, sizeof(write_buffer));
    Writer<FileWriteStream> writer(out_stream);

    output.Accept(writer);
    std::cout << std::endl;
}
