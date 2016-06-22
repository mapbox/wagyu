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

    // todo catch parsing errors

    for (SizeType i = 0; i < document.Size(); ++i) {
        mapbox::geometry::linear_ring<value_type> lr;

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



int main(int argc, char* const argv[]) {
    if (argc < 3) {
        std::cout << "Error: too few parameters\n" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  ./fixture-test ./path/to/subject.json ./path/to/object.json\n" << std::endl;
        std::cout << "Options:" << std::endl;
        std::cout << "  -t     type of operation (default: union)\n" << std::endl;
        return -1;
    }

    clipper<value_type> clipper;
    std::string type = "union";

    if (argc == 5 && strcmp(argv[1],"-t") == 0) {
        type = argv[2];
        parse_file(argv[3], clipper, polygon_type_subject);
        parse_file(argv[4], clipper, polygon_type_clip);
    } else if (argc == 5 && strcmp(argv[3], "-t") == 0) {
        type = argv[4];
        parse_file(argv[1], clipper, polygon_type_subject);
        parse_file(argv[2], clipper, polygon_type_clip);
    } else {
        parse_file(argv[1], clipper, polygon_type_subject);
        parse_file(argv[2], clipper, polygon_type_clip);
    }

    clip_type operation;

    if (type.compare("union") == 0) {
        operation = clip_type_union;
    } else if (type.compare("intersection") == 0) {
        operation = clip_type_intersection;
    } else if (type.compare("difference") == 0) {
        operation = clip_type_difference;
    } else if (type.compare("x_or") == 0) {
        operation = clip_type_x_or;
    } else {
        operation = clip_type_union;
    }

    std::vector<mapbox::geometry::polygon<value_type>> solution;
    clipper.execute(operation, solution, fill_type_even_odd, fill_type_even_odd);

    Document output;
    polys_to_json(output, solution);

    char write_buffer[65536];
    FileWriteStream out_stream(stdout, write_buffer, sizeof(write_buffer));
    Writer<FileWriteStream> writer(out_stream);

    output.Accept(writer);
}

