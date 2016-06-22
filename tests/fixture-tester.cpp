#include <stdio.h>
#include <ostream>
#include <iostream>
#include <mapbox/geometry/polygon.hpp>
#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <mapbox/geometry/wagyu/wagyu.hpp>


using namespace rapidjson;
using namespace mapbox::geometry::wagyu;
using value_type = std::int64_t;

struct Options {
    clip_type operation = clip_type_union;
    char* subject_file;
    char* clip_file;
} options;


void parse_file(const char* file_path, 
    clipper<value_type>& clipper, 
    polygon_type polytype
) {
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
    for (SizeType i = 0; i < document.Size(); ++i) {
        mapbox::geometry::linear_ring<value_type> lr;

        if (!document[i].IsArray()) {
            throw std::runtime_error("A ring (in " + std::string(file_path) + ") is not a valid json array");
        }
        for (SizeType j = 0; j < document[i].Size(); ++j) {
            lr.push_back({document[i][j][0].GetInt(), document[i][j][1].GetInt()});
        } 
        clipper.add_ring(lr, polytype);
    }

    fclose(file);
}

void polys_to_json(Document& output, std::vector<mapbox::geometry::polygon<value_type>> solution) {
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


int main(int argc, char* const argv[]) {
    if (argc < 3) {
        std::cout << "Error: too few parameters\n" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  ./fixture-test ./path/to/subject.json ./path/to/object.json\n" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -t     type of operation (default: union)\n" << std::endl;
        return -1;
    }
    parse_options(argc, argv);

    clipper<value_type> clipper;
    parse_file(options.subject_file, clipper, polygon_type_subject);
    parse_file(options.clip_file, clipper, polygon_type_clip);

    std::vector<mapbox::geometry::polygon<value_type>> solution;
    clipper.execute(options.operation, solution, fill_type_even_odd, fill_type_even_odd);

    Document output;
    polys_to_json(output, solution);

    char write_buffer[65536];
    FileWriteStream out_stream(stdout, write_buffer, sizeof(write_buffer));
    Writer<FileWriteStream> writer(out_stream);

    output.Accept(writer);
}

