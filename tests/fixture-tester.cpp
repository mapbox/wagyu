#include <stdio.h>
#include <ostream>
#include <iostream>
#include <mapbox/geometry/polygon.hpp>
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <mapbox/geometry/wagyu/wagyu.hpp>


using namespace rapidjson;
using namespace mapbox::geometry::wagyu;
using value_type = std::int64_t;


void parse_file(const char* file_path, 
    clipper<value_type>& clipper, 
    polygon_type polytype
) {
    FILE* file = fopen(file_path, "r");
    char read_buffer[65536];
    FileReadStream in_stream(file, read_buffer, sizeof(read_buffer));
    Document document;
    document.ParseStream<0, UTF8<>, FileReadStream>(in_stream);


    std::cout << document.IsArray() << std::endl;
    for (SizeType i = 0; i < document.Size(); ++i) {
        mapbox::geometry::linear_ring<value_type> lr;

        for (SizeType j = 0; j < document[i].Size(); ++j) {
            lr.push_back({document[i][j][0].GetInt(), document[i][j][1].GetInt()});
        } 
        clipper.add_ring(lr, polytype);
    }
}

int main(int argc, char* const argv[]) {
    if (argc < 2) return -1;

    clipper<value_type> clipper;

    parse_file(argv[1], clipper, polygon_type_subject);
    parse_file(argv[1], clipper, polygon_type_clip);


    std::vector<mapbox::geometry::polygon<value_type>> solution;
    clipper.execute(clip_type_union, solution, fill_type_even_odd, fill_type_even_odd);
}

