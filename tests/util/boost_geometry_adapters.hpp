#include <mapbox/geometry/line_string.hpp>
#include <mapbox/geometry/multi_polygon.hpp>
#include <mapbox/geometry/point.hpp>
#include <mapbox/geometry/polygon.hpp>

#include <boost/geometry.hpp>
#include <boost/range/iterator_range_core.hpp>

#include <cassert>

namespace boost {
namespace geometry {
namespace traits {

template <typename CoordinateType>
struct tag<mapbox::geometry::point<CoordinateType>> {
    using type = point_tag;
};

template <typename CoordinateType>
struct coordinate_type<mapbox::geometry::point<CoordinateType>> {
    using type = CoordinateType;
};

template <typename CoordinateType>
struct coordinate_system<mapbox::geometry::point<CoordinateType>> {
    using type = boost::geometry::cs::cartesian;
};

template <typename CoordinateType>
struct dimension<mapbox::geometry::point<CoordinateType>> : boost::mpl::int_<2> {};

template <typename CoordinateType>
struct access<mapbox::geometry::point<CoordinateType>, 0> {
    static CoordinateType get(mapbox::geometry::point<CoordinateType> const& p) {
        return p.x;
    }

    static void set(mapbox::geometry::point<CoordinateType>& p, CoordinateType x) {
        p.x = x;
    }
};

template <typename CoordinateType>
struct access<mapbox::geometry::point<CoordinateType>, 1> {
    static CoordinateType get(mapbox::geometry::point<CoordinateType> const& p) {
        return p.y;
    }

    static void set(mapbox::geometry::point<CoordinateType>& p, CoordinateType y) {
        p.y = y;
    }
};

template <typename CoordinateType>
struct tag<mapbox::geometry::linear_ring<CoordinateType>> {
    using type = ring_tag;
};

template <typename CoordinateType>
struct point_order<mapbox::geometry::linear_ring<CoordinateType>> {
    static const order_selector value = counterclockwise;
};

template <typename CoordinateType>
struct tag<mapbox::geometry::polygon<CoordinateType>> {
    using type = polygon_tag;
};

template <typename CoordinateType>
struct ring_mutable_type<mapbox::geometry::polygon<CoordinateType>> {
    using type = mapbox::geometry::linear_ring<CoordinateType>&;
};

template <typename CoordinateType>
struct ring_const_type<mapbox::geometry::polygon<CoordinateType>> {
    using type = mapbox::geometry::linear_ring<CoordinateType> const&;
};

template <typename CoordinateType>
struct interior_mutable_type<mapbox::geometry::polygon<CoordinateType>> {
    using type =
        boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::iterator>;
};

template <typename CoordinateType>
struct interior_const_type<mapbox::geometry::polygon<CoordinateType>> {
    using type =
        boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::const_iterator>;
};

template <typename CoordinateType>
struct exterior_ring<mapbox::geometry::polygon<CoordinateType>> {
    static mapbox::geometry::linear_ring<CoordinateType>&
    get(mapbox::geometry::polygon<CoordinateType>& p) {
        return p.at(0);
    }

    static mapbox::geometry::linear_ring<CoordinateType> const&
    get(mapbox::geometry::polygon<CoordinateType> const& p) {
        return p.at(0);
    }
};

template <typename CoordinateType>
struct interior_rings<mapbox::geometry::polygon<CoordinateType>> {
    static boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::iterator>
    get(mapbox::geometry::polygon<CoordinateType>& p) {
        return boost::make_iterator_range(p.begin() + 1, p.end());
    }

    static boost::iterator_range<typename mapbox::geometry::polygon<CoordinateType>::const_iterator>
    get(mapbox::geometry::polygon<CoordinateType> const& p) {
        return boost::make_iterator_range(p.begin() + 1, p.end());
    }
};

template <typename CoordinateType>
struct tag<mapbox::geometry::multi_polygon<CoordinateType>> {
    using type = multi_polygon_tag;
};
}
}
}
