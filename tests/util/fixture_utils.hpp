#include <cstdio>
#include <iostream>
#include <mapbox/geometry/polygon.hpp>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/filewritestream.h>
#include <rapidjson/writer.h>

template <typename T>
mapbox::geometry::polygon<T> fixture_file_to_polygon(const char* file_path) {
    // todo safety checks opening files
    FILE* file = fopen(file_path, "r");
    char read_buffer[65536];
    rapidjson::FileReadStream in_stream(file, read_buffer, sizeof(read_buffer));
    rapidjson::Document document;
    document.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(in_stream);

    if (!document.IsArray()) {
        throw std::runtime_error(("Input file (" + std::string(file_path) + ") is not valid json"));
    }
    // todo catch parsing errors
    mapbox::geometry::polygon<T> poly;
    for (rapidjson::SizeType i = 0; i < document.Size(); ++i) {
        mapbox::geometry::linear_ring<T> lr;

        if (!document[i].IsArray()) {
            throw std::runtime_error("A ring (in " + std::string(file_path) + ") is not a valid json array");
        }
        for (rapidjson::SizeType j = 0; j < document[i].Size(); ++j) {
            lr.push_back({ document[i][j][0].GetInt(), document[i][j][1].GetInt() });
        }
        poly.emplace_back(lr);
    }
    fclose(file);
    return poly;
}
