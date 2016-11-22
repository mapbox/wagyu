#include <mapbox/geometry/wagyu/wagyu.hpp>
#include <stdio.h>

int main() {
    mapbox::geometry::linear_ring<long long> ring;

    ring.push_back(mapbox::geometry::point<long long>(0, 0));
    ring.push_back(mapbox::geometry::point<long long>(1, 0));
    ring.push_back(mapbox::geometry::point<long long>(1, 1));
    ring.push_back(mapbox::geometry::point<long long>(0, 1));
    ring.push_back(mapbox::geometry::point<long long>(0, 0));

    mapbox::geometry::wagyu::wagyu<long long> wagyu;
    wagyu.add_ring(ring);

    mapbox::geometry::multi_polygon<long long> result;
    wagyu.execute(mapbox::geometry::wagyu::clip_type_union, result,
                  mapbox::geometry::wagyu::fill_type_positive,
                  mapbox::geometry::wagyu::fill_type_positive);

    printf("positive:\n");
    for (size_t i = 0; i < result.size(); i++) {
        for (size_t j = 0; j < result[i].size(); j++) {
            for (size_t k = 0; k < result[i][j].size(); k++) {
                printf("%lld,%lld\n", result[i][j][k].x, result[i][j][k].y);
            }
        }
    }

    result.clear();
    wagyu.execute(mapbox::geometry::wagyu::clip_type_union, result,
                  mapbox::geometry::wagyu::fill_type_even_odd,
                  mapbox::geometry::wagyu::fill_type_even_odd);

    printf("even-odd:\n");
    for (size_t i = 0; i < result.size(); i++) {
        for (size_t j = 0; j < result[i].size(); j++) {
            for (size_t k = 0; k < result[i][j].size(); k++) {
                printf("%lld,%lld\n", result[i][j][k].x, result[i][j][k].y);
            }
        }
    }

    return 0;
}
