#pragma once
#include "../tests/util/fixture_utils.hpp"
#include "angus.hpp"
#include <benchmark/benchmark.h>
#include <boost/filesystem.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>

auto BM_wagyu_fixture_union = [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
    auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
    auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

    while (state.KeepRunning()) {
        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        clipper.add_polygon(poly_subject, mapbox::geometry::wagyu::polygon_type_subject);
        clipper.add_polygon(poly_clip, mapbox::geometry::wagyu::polygon_type_clip);
        mapbox::geometry::multi_polygon<std::int64_t> solution;
        clipper.execute(mapbox::geometry::wagyu::clip_type_union, solution, mapbox::geometry::wagyu::fill_type_even_odd,
                        mapbox::geometry::wagyu::fill_type_even_odd);
    }
};

auto BM_wagyu_fixture_intersection =
    [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
        auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
        auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

        while (state.KeepRunning()) {
            mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
            clipper.add_polygon(poly_subject, mapbox::geometry::wagyu::polygon_type_subject);
            clipper.add_polygon(poly_clip, mapbox::geometry::wagyu::polygon_type_clip);
            mapbox::geometry::multi_polygon<std::int64_t> solution;
            clipper.execute(mapbox::geometry::wagyu::clip_type_intersection, solution,
                            mapbox::geometry::wagyu::fill_type_even_odd, mapbox::geometry::wagyu::fill_type_even_odd);
        }
    };

auto BM_wagyu_fixture_difference =
    [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
        auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
        auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

        while (state.KeepRunning()) {
            mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
            clipper.add_polygon(poly_subject, mapbox::geometry::wagyu::polygon_type_subject);
            clipper.add_polygon(poly_clip, mapbox::geometry::wagyu::polygon_type_clip);
            mapbox::geometry::multi_polygon<std::int64_t> solution;
            clipper.execute(mapbox::geometry::wagyu::clip_type_difference, solution,
                            mapbox::geometry::wagyu::fill_type_even_odd, mapbox::geometry::wagyu::fill_type_even_odd);
        }
    };

auto BM_wagyu_fixture_x_or = [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
    auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
    auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

    while (state.KeepRunning()) {
        mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
        clipper.add_polygon(poly_subject, mapbox::geometry::wagyu::polygon_type_subject);
        clipper.add_polygon(poly_clip, mapbox::geometry::wagyu::polygon_type_clip);
        mapbox::geometry::multi_polygon<std::int64_t> solution;
        clipper.execute(mapbox::geometry::wagyu::clip_type_x_or, solution, mapbox::geometry::wagyu::fill_type_even_odd,
                        mapbox::geometry::wagyu::fill_type_even_odd);
    }
};

inline void process_polynode_branch(ClipperLib::PolyNode* polynode, mapbox::geometry::multi_polygon<std::int64_t>& mp) {
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

auto BM_angus_fixture_union = [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
    auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
    auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

    while (state.KeepRunning()) {
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
        clipper.Execute(ClipperLib::ctUnion, polygons, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
        clipper.Clear();
        mapbox::geometry::multi_polygon<std::int64_t> solution;

        for (auto* polynode : polygons.Childs) {
            process_polynode_branch(polynode, solution);
        }
    }
};

auto BM_angus_fixture_intersection =
    [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
        auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
        auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

        while (state.KeepRunning()) {
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
            clipper.Execute(ClipperLib::ctIntersection, polygons, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
            clipper.Clear();
            mapbox::geometry::multi_polygon<std::int64_t> solution;

            for (auto* polynode : polygons.Childs) {
                process_polynode_branch(polynode, solution);
            }
        }
    };

auto BM_angus_fixture_difference =
    [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
        auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
        auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

        while (state.KeepRunning()) {
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
            clipper.Execute(ClipperLib::ctDifference, polygons, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
            clipper.Clear();
            mapbox::geometry::multi_polygon<std::int64_t> solution;

            for (auto* polynode : polygons.Childs) {
                process_polynode_branch(polynode, solution);
            }
        }
    };

auto BM_angus_fixture_x_or = [](benchmark::State& state, std::string subject_filename, std::string clip_filename) {
    auto poly_subject = fixture_file_to_polygon<std::int64_t>(subject_filename.c_str());
    auto poly_clip = fixture_file_to_polygon<std::int64_t>(clip_filename.c_str());

    while (state.KeepRunning()) {
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
        clipper.Execute(ClipperLib::ctXor, polygons, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);
        clipper.Clear();
        mapbox::geometry::multi_polygon<std::int64_t> solution;

        for (auto* polynode : polygons.Childs) {
            process_polynode_branch(polynode, solution);
        }
    }
};

inline void register_fixtures() {
    boost::filesystem::path fixture_directory("./tests/output-polyjson");
    boost::filesystem::path clip_file("./tests/fixtures/clip-clockwise-square.json");
    for (auto subject : boost::filesystem::directory_iterator(fixture_directory)) {
        if (!boost::filesystem::is_regular_file(subject)) {
            continue;
        }
        std::string union_name = std::string("f/") + subject.path().filename().string() + std::string("/union/wagyu");
        std::string union_name2 = std::string("f/") + subject.path().filename().string() + std::string("/union/angus");
        std::string intersection_name =
            std::string("f/") + subject.path().filename().string() + std::string("/intersection/wagyu");
        std::string intersection_name2 =
            std::string("f/") + subject.path().filename().string() + std::string("/intersection/angus");
        std::string difference_name =
            std::string("f/") + subject.path().filename().string() + std::string("/difference/wagyu");
        std::string difference_name2 =
            std::string("f/") + subject.path().filename().string() + std::string("/difference/angus");
        std::string x_or_name = std::string("f/") + subject.path().filename().string() + std::string("/x_or/wagyu");
        std::string x_or_name2 = std::string("f/") + subject.path().filename().string() + std::string("/x_or/angus");

        benchmark::RegisterBenchmark(union_name.c_str(), BM_wagyu_fixture_union, subject.path().native(),
                                     clip_file.native());
        benchmark::RegisterBenchmark(union_name2.c_str(), BM_angus_fixture_union, subject.path().native(),
                                     clip_file.native());

        benchmark::RegisterBenchmark(intersection_name.c_str(), BM_wagyu_fixture_intersection, subject.path().native(),
                                     clip_file.native());
        benchmark::RegisterBenchmark(intersection_name2.c_str(), BM_angus_fixture_intersection, subject.path().native(),
                                     clip_file.native());

        benchmark::RegisterBenchmark(difference_name.c_str(), BM_wagyu_fixture_difference, subject.path().native(),
                                     clip_file.native());
        benchmark::RegisterBenchmark(difference_name2.c_str(), BM_angus_fixture_difference, subject.path().native(),
                                     clip_file.native());

        benchmark::RegisterBenchmark(x_or_name.c_str(), BM_wagyu_fixture_x_or, subject.path().native(),
                                     clip_file.native());
        benchmark::RegisterBenchmark(x_or_name2.c_str(), BM_angus_fixture_x_or, subject.path().native(),
                                     clip_file.native());
    }
}
