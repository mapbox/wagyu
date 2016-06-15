#pragma once

#include <mapbox/geometry/point.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {
// We can possibly get rid of this perhaps at somepoint, just migrated
// for debugging purposes
template <typename T>
struct box {
    using value_type = T;
    value_type left;
    value_type top;
    value_type right;
    value_type bottom;
};
}
}
}
