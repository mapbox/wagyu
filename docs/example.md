## Example Use of Wagyu

```
    mapbox::geometry::wagyu::wagyu<std::int64_t> clipper;
    mapbox::geometry::polygon<std::int64_t> polygon;
    mapbox::geometry::linear_ring<std::int64_t> ring0_0;
    ring0_0.push_back({ -79102, 0 });
    ring0_0.push_back({ -70312, -55285 });
    ring0_0.push_back({ 85254, -30747 });
    ring0_0.push_back({ 58008, 80592 });
    ring0_0.push_back({ -79102, 0 });
    polygon0.push_back(ring0_0);

    mapbox::geometry::linear_ring<T> ring0_1;
    ring0_1.push_back({ 44824, 42149 });
    ring0_1.push_back({ 51855, -21089 });
    ring0_1.push_back({ -65918, -32502 });
    ring0_1.push_back({ -50098, 4394 });
    ring0_1.push_back({ 44824, 42149 });
    polygon0.push_back(ring0_1);

    clipper.add_polygon(polygon0, polygon_type::polygon_type_subject);

    mapbox::geometry::polygon<T> polygon1;
    mapbox::geometry::linear_ring<T> ring1_0;
    ring1_0.push_back({ 31201, 8349 });
    ring1_0.push_back({ 4834, 19771 });
    ring1_0.push_back({ -25488, -6592 });
    ring1_0.push_back({ 10547, -19771 });
    ring1_0.push_back({ 31201, 8349 });
    polygon1.push_back(ring1_0);

    clipper.add_polygon(polygon1, polygon_type::polygon_type_clip);

    mapbox::geometry::polygon<T> polygon2;
    mapbox::geometry::linear_ring<T> ring2_0;
    ring2_0.push_back({ -40430, -3076 });
    ring2_0.push_back({ -26367, -18454 });
    ring2_0.push_back({ 34277, -4834 });
    ring2_0.push_back({ 33838, 17136 });
    ring2_0.push_back({ -40430, -3076 });
    polygon2.push_back(ring2_0);

    clipper.add_polygon(polygon2, polygon_type::polygon_type_subject);

    mapbox::geometry::multi_polygon<T> solution;
    clipper.execute(mapbox::geometry::wagyu::clip_type_union, 
                    solution, 
                    mapbox::geometry::wagyu::fill_type_even_odd, 
                    mapbox::geometry::wagyu::fill_type_even_odd);
```

