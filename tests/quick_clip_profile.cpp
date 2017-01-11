#include <iostream>
#include <mapbox/geometry/wagyu/quick_clip.hpp>
#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

using namespace mapbox::geometry::wagyu;
using T = std::int64_t;

void check(int pass) {
    srand(0);
    clock_t before = clock();

    for (size_t i = 0; i < 10000; i++) {
        mapbox::geometry::point<T> p1 = { 100, 100 };
        mapbox::geometry::point<T> p2 = { 200, 200 };
        mapbox::geometry::box<T> bbox(p1, p2);

        mapbox::geometry::linear_ring<T> lr;
        size_t n = rand() % 500 + 4;
        for (size_t j = 0; j < n; j++) {
            lr.push_back(mapbox::geometry::point<T>(rand() % 300, rand() % 300));
        }
        lr.push_back(lr[0]);

        if (pass == 0) {
            optional_linear_ring<T> out =
                mapbox::geometry::wagyu::quick_clip::quick_lr_clip(lr, bbox);
        } else {
            optional_linear_ring<T> out =
                mapbox::geometry::wagyu::quick_clip::quick_lr_clip1(lr, bbox);
        }
    }

    clock_t after = clock();

    if (pass == 0) {
        std::cout << "quick_lr_clip";
    } else {
        std::cout << "quick_lr_clip1";
    }

    std::cout << ": " << (after - before) << "\n";
}

int main() {
    check(0);
    check(1);
    return 0;
}
