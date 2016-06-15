#pragma once

#include <queue>

#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {
template <typename T>
using scanbeam_list = std::priority_queue<T>;

template <typename T>
bool pop_scanbeam(scanbeam_list<T>& scanbeam, T& Y) {
    if (scanbeam.empty()) {
        return false;
    }
    Y = scanbeam.top();
    scanbeam.pop();
    while (!scanbeam.empty() && Y == scanbeam.top()) {
        // Pop duplicates.
        scanbeam.pop();
    }
    return true;
}
}
}
}
