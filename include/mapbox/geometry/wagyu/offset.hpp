#pragma once

#include <mapbox/geometry/wagyu/wagyu.hpp>

#include <mapbox/geometry/linear_ring.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

enum corner_type : std::uint8_t
{
    corner_square_type = 0,
    corner_round_type,
    corner_miter_type
};

/*****************************************************
*
*                  b (offset point)
*                  @
*                 /|\
*                / | \      ß = angle formed by points (lam)
*               /  |  \
*              /   |   @ l
*             /    |    \
*            /     |   _ @ c <--- A RIGHT ANGLE
*           /      | _/   \       pardon the ASCI
*          /     a @/      \      art.
*         /       / \       \
*        /       / Ø \       \    length(ac) == offset
*       /       /     \       \
*      /       /       @ m     \
*******************************************************/
// When 'mitering' offset polygons, the maximum distance point 'b' can be from 
// point 'a' is set by 'limit' where limit is a multiple of 'offset'. Therefore, 
// for any given angle we need to know if length(ab) > limit * offset.
//
// Find the largest angle ß (or smallest Ø since Ø = pi - ß) for a given 
// limit, expressing ß as sin(ß) or cos(ß) since these can easily be derived 
// from cross or dot products respectively. 
//
// angle(abc) = Ø/2 
// length(ab) = limit * offset
// length(ac) = offset
// sin(Ø/2) = offset / (limit * offset) = 1 / limit
// Given that sin(Ø/2) = sqrt((1-cos(Ø))/2) **
// 1 / limit = sqrt((1-cos(Ø))/2)
// limit = sqrt(2 / (1-cos(Ø)))
// 1-cos(Ø) = 2 / sqr(limit) 
// Since Ø = pi - ß ...
// 1-cos(pi - ß) = 2 / sqr(limit)
// and given cos(pi-ß) = -cos(ß) ** ... 
// 1+cos(ß) = 2 / sqr(limit) 
// cos(ß) = 2 / sqr(limit) - 1

// Example: if miter limit = 2 (ie 2 times offset) then cos(ß) = (2 / 4) -1 = -0.5 
// and so ß = 120 degrees. Therefore, when ß > 120 deg. (or Ø < 60 deg.), the 
// distance point 'b' would be from point 'a' would exceed the limit.


template <typename T>
geometry::point<double> unit_normal(geometry::point<T> const& v0, geometry::point<T> const& v1) {
    // Determine dy and dx
    double dx = static_cast<double>(v0.x - v1.x); 
    double dy = static_cast<double>(v0.y - v1.y);
    double ds = std::sqrt(dx*dx + dy*dy); // magnitude of segment vector
    dx = dx / ds;
    dy = dy / ds;
    // Unit normal vector to the LEFT.
    return point<double>(-dy, dx);
}

template <typename T>
geometry::point<T> offset_dx_dy(geometry::point<T> const& pt, double x, double y) {
    return geometry::point<T>(v.x + static_cast<T>(std::llround(x)), 
                              v.y + static_cast<T>(std::llround(y)));
}

template <>
geometry::point<double> offset_dx_dy<double>(point<double> const& v, double x, double y) {
    return point<double>(v.x + x, v.y + y);
}

template <T>
geometry::point<T> offset_point(geometry::point<T> const& v0,
                      double offset,
                      geometry::point<double> const& seg0_unit_normal) {
    // Solve for the point at p0 that is offset from
    // the point at v0. The point is offset by the distance
    // "offset" on the unit vector normal to the segment seg0.
    /*******************************************************
    *
    *               o 
    *              /
    *             /
    *    p0      / seg0
    *     @     /
    *          o
    *          v0
    * 
    * Note: showing a positive offset here.
    *******************************************************/
    
    return offset_dx_dy(v0, 
                        offset * seg0_unit_normal.x, 
                        offset * seg0_unit_normal.y);
}

template <typename T, typename Container>
void offset_corner_miter(geometry::point<T> const& v1,
                         double offset,
                         double m_limit_sqr_over_two,
                         geometry::point<double> const& seg0_unit_normal,
                         geometry::point<double> const& seg1_unit_normal,
                         Container & container) {
    /***********************************************
    * We are creating the offset section around the 
    * corner of a joint of two segments as shown below:
    *            
    *                 v1    seg1   v2
    *                o--------------o
    *               / 
    *              /               
    *             / seg0               
    *            /                 
    *           o                 
    *           v0                 
    *
    * The side of the offset segments required will
    * will be based on the offset being negative
    * or positive.
    ************************************************/
    
    // We can find the angle between the two unit normals
    // relative to the v1 in order to solve for the angle
    // and offset set of points. We can call the angle
    // between the two unit vectors to be "theta".

    // The determinate of the two unit normal vectors is equal 
    // to "magnitude" of sin(Ø).
    double m_sin_theta = seg0_unit_normal.x * seg1_unit_normal.y - seg0_unit_normal.y * seg1_unit_normal.x;
    double m_cos_theta = seg0_unit_normal.x * seg1_unit_normal.x + seg0_unit_normal.y * seg1_unit_normal.y;
    
    // If absolute value of m_sin_theta multiplied by offset is less then 1, we know that 
    // the hypotenuse calculated using the two angles will be less then the value of our offset
    if (std::fabs(m_sin_theta * offset) < 1.0) {
        // Now we need to check that the angle is very near to either 2pi or 0
        // and not on the other side of the unit circle. Therefore, we need
        // to check if cos would put it in quadrants II or III. 
        if (m_cos_theta > 0.0) {
            // We know this is in the I or IV quadrant, therefore this is a very slight
            // angle. Therefore we should simply just offset from unit normal of seg1.
            // (seg0 would work as well more then likely)
            // This is basically a straight line
            container.push_back(offset_point(v1, offset, seg1_unit_normal));
            return;
        }
    }
    // It seems confusing to cap sin at this
    // point but the purpose is that we are
    // now calculating ß rather then Ø. This
    // will be used in the case we have 
    // a square or round corner.
    if (m_sin_theta > 1.0) {
        m_sin_theta = 1.0;
    } else if (m_sin_theta < -1.0) {
        m_sin_theta = -1.0;
    }

    // Next we need to check if the angle would be concave
    // The previous capping of sine won't change the result of this
    if (m_sin_theta * offset < 0.0) {
        // If the angle is concave we add three offset points
        // it is expected that they will cause intersections
        // but a winding order calculation later will fix these
        // sort of issues.
        offset_corner_concave(v1, offset, seg0_unit_normal, seg1_unit_normal, container);
        return;
    }
    
    // The angle is going to be convex so we can decide the type of corner to apply
    double cos_beta_plus_1 = 1.0 + m_cos_theta;
    if (cos_beta_plus_1 >= m_limit_sqr_over_two) {
        offset_corner_point_miter(cos_beta_plus_1,
                                  v1,
                                  offset,
                                  seg0_unit_normal,
                                  seg1_unit_normal,
                                  container);
    } else {
        offset_corner_point_square(m_sin_theta,
                                   m_cos_theta,
                                   v1,
                                   offset,
                                   seg0_unit_normal,
                                   seg1_unit_normal,
                                   container);
    }
}

template <typename T, typename Container>
void offset_corner_square(geometry::point<T> const& v1,
                          double offset,
                          geometry::point<double> const& seg0_unit_normal,
                          geometry::point<double> const& seg1_unit_normal,
                          Container & container) {
    /***********************************************
    * We are creating the offset section around the 
    * corner of a joint of two segments as shown below:
    *            
    *                 v1    seg1   v2
    *                o--------------o
    *               / 
    *              /               
    *             / seg0               
    *            /                 
    *           o                 
    *           v0                 
    *
    * The side of the offset segments required will
    * will be based on the offset being negative
    * or positive.
    ************************************************/
    
    // We can find the angle between the two unit normals
    // relative to the v1 in order to solve for the angle
    // and offset set of points. We can call the angle
    // between the two unit vectors to be "theta".

    // The determinate of the two unit normal vectors is equal 
    // to "magnitude" of sin(Ø).
    double m_sin_theta = seg0_unit_normal.x * seg1_unit_normal.y - seg0_unit_normal.y * seg1_unit_normal.x;
    double m_cos_theta = seg0_unit_normal.x * seg1_unit_normal.x + seg0_unit_normal.y * seg1_unit_normal.y;
    
    // If absolute value of m_sin_theta multiplied by offset is less then 1, we know that 
    // the hypotenuse calculated using the two angles will be less then the value of our offset
    if (std::fabs(m_sin_theta * offset) < 1.0) {
        // Now we need to check that the angle is very near to either 2pi or 0
        // and not on the other side of the unit circle. Therefore, we need
        // to check if cos would put it in quadrants II or III. 
        if (m_cos_theta > 0.0) {
            // We know this is in the I or IV quadrant, therefore this is a very slight
            // angle. Therefore we should simply just offset from unit normal of seg1.
            // (seg0 would work as well more then likely)
            // This is basically a straight line
            container.push_back(offset_point(v1, offset, seg1_unit_normal));
            return;
        }
    }
    // It seems confusing to cap sin at this
    // point but the purpose is that we are
    // now calculating ß rather then Ø. This
    // will be used in the case we have 
    // a square or round corner.
    if (m_sin_theta > 1.0) {
        m_sin_theta = 1.0;
    } else if (m_sin_theta < -1.0) {
        m_sin_theta = -1.0;
    }

    // Next we need to check if the angle would be concave
    // The previous capping of sine won't change the result of this
    if (m_sin_theta * offset < 0.0) {
        // If the angle is concave we add three offset points
        // it is expected that they will cause intersections
        // but a winding order calculation later will fix these
        // sort of issues.
        offset_corner_concave(v1, offset, seg0_unit_normal, seg1_unit_normal, container);
        return;
    }
    
    // The angle is going to be convex so we can decide the type of corner to apply
    offset_corner_square(m_sin_theta,
                         m_cos_theta,
                         v1,
                         offset,
                         seg0_unit_normal,
                         seg1_unit_normal,
                         container);
}

template <typename T, typename Container>
void offset_corner_round(geometry::point<T> const& v1,
                         double offset,
                         double steps_per_radian,
                         double steps_cos,
                         double steps_sin,
                         geometry::point<double> const& seg0_unit_normal,
                         geometry::point<double> const& seg1_unit_normal,
                         Container & container) {
    /***********************************************
    * We are creating the offset section around the 
    * corner of a joint of two segments as shown below:
    *            
    *                 v1    seg1   v2
    *                o--------------o
    *               / 
    *              /               
    *             / seg0               
    *            /                 
    *           o                 
    *           v0                 
    *
    * The side of the offset segments required will
    * will be based on the offset being negative
    * or positive.
    ************************************************/
    
    // We can find the angle between the two unit normals
    // relative to the v1 in order to solve for the angle
    // and offset set of points. We can call the angle
    // between the two unit vectors to be "theta".

    // The determinate of the two unit normal vectors is equal 
    // to "magnitude" of sin(Ø).
    double m_sin_theta = seg0_unit_normal.x * seg1_unit_normal.y - seg0_unit_normal.y * seg1_unit_normal.x;
    double m_cos_theta = seg0_unit_normal.x * seg1_unit_normal.x + seg0_unit_normal.y * seg1_unit_normal.y;
    
    // If absolute value of m_sin_theta multiplied by offset is less then 1, we know that 
    // the hypotenuse calculated using the two angles will be less then the value of our offset
    if (std::fabs(m_sin_theta * offset) < 1.0) {
        // Now we need to check that the angle is very near to either 2pi or 0
        // and not on the other side of the unit circle. Therefore, we need
        // to check if cos would put it in quadrants II or III. 
        if (m_cos_theta > 0.0) {
            // We know this is in the I or IV quadrant, therefore this is a very slight
            // angle. Therefore we should simply just offset from unit normal of seg1.
            // (seg0 would work as well more then likely)
            // This is basically a straight line
            container.push_back(offset_point(v1, offset, seg1_unit_normal));
            return;
        }
    }
    // It seems confusing to cap sin at this
    // point but the purpose is that we are
    // now calculating ß rather then Ø. This
    // will be used in the case we have 
    // a square or round corner.
    if (m_sin_theta > 1.0) {
        m_sin_theta = 1.0;
    } else if (m_sin_theta < -1.0) {
        m_sin_theta = -1.0;
    }

    // Next we need to check if the angle would be concave
    // The previous capping of sine won't change the result of this
    if (m_sin_theta * offset < 0.0) {
        // If the angle is concave we add three offset points
        // it is expected that they will cause intersections
        // but a winding order calculation later will fix these
        // sort of issues.
        offset_corner_concave(v1, offset, seg0_unit_normal, seg1_unit_normal, container);
        return;
    }
    
    // The angle is going to be convex so we can decide the type of corner to apply
    offset_corner_point_round(m_sin_theta,
                              m_cos_theta,
                              v1,
                              offset,
                              steps_per_radian,
                              steps_cos,
                              steps_sin,
                              seg0_unit_normal,
                              seg1_unit_normal,
                              container);
}

template <typename T, typename Container>
void offset_corner_concave(geometry::point<T> const& v1,
                           double offset,
                           geometry::point<double> const& seg0_unit_normal,
                           geometry::point<double> const& seg1_unit_normal,
                           Container & container) {
    container.push_back(offset_point(v1, offset, seg0_unit_normal));
    container.push_back(v1);
    container.push_back(offset_point(v1, offset, seg1_unit_normal));
}

template <typename T, typename Container>
void offset_corner_point_square(double m_sin_theta,
                                double m_cos_theta,
                                geometry::point<T> const& v1,
                                double offset,
                                geometry::point<double> const& seg0_unit_normal,
                                geometry::point<double> const& seg1_unit_normal,
                                Container & container) {
    // We want to calculate tan(ß/4) as this gives us the distance
    // to the point where the squaring will start.
    
    double beta = std::atan2(m_sin_theta, m_cos_theta);
    double tan_beta_4 = std::tan(beta / 4);
    
    container.push_back(offset_dx_dy(
                            v1,
                            offset,
                            offset * (seg0_unit_normal.x - seg0_unit_normal.y * tan_beta_4),
                            offset * (seg0_unit_normal.y + seg0_unit_normal.x * tan_beta_4)
                        ));
    container.push_back(offset_dx_dy(
                            v1,
                            offset,
                            offset * (seg1_unit_normal.x + seg1_unit_normal.y * tan_beta_4),
                            offset * (seg1_unit_normal.y - seg1_unit_normal.x * tan_beta_4)
                        ));
}

template <typename T, typename Container>
void offset_corner_point_miter(double cos_beta_plus_one,
                               geometry::point<T> const& v1,
                               double offset,
                               geometry::point<double> const& seg0_unit_normal,
                               geometry::point<double> const& seg1_unit_normal,
                               Container & container) {
    double q = offset / cos_beta_plus_one;
    container.push_back(offset_dx_dy(
                            v1,
                            offset,
                            q * (seg0_unit_normal.x + seg1_unit_normal.x),
                            q * (seg0_unit_normal.y + seg1_unit_normal.y)
                        ));
}

template <typename T, typename Container>
void offset_corner_point_round(double m_sin_theta,
                               double m_cos_theta,
                               geometry::point<T> const& v1,
                               double offset,
                               double steps_per_radian,
                               double steps_cos,
                               double steps_sin,
                               geometry::point<double> const& seg0_unit_normal,
                               geometry::point<double> const& seg1_unit_normal,
                               Container & container)
{
    double beta = std::atan2(m_sin_theta, m_cos_theta);
    std::size_t steps = static_cast<std::size_t>(std::round(steps_per_radian_ * std::fabs(beta)));
    
    // Always have at least one step
    if (steps < 1) {
        steps = 1;
    }
    
    // Vector unit vector components to the position around the circle
    // starting at the unit normal vector of seg0.
    double vec_x = seg0_unit_normal.x;
    double vec_y = seg0_unit_normal.y;

    // Add the first position.
    container.push_back(offset_point(v1, offset, seg0_unit_normal));

    for (std::size_t i = 1; i < steps; ++i) { 
        // Calculate next vector position
        double prev_vec_x = vec_x;
        vec_x = prev_vec_x * steps_cos - vec_y * steps_sin;
        vec_y = prev_vec_x * steps_sin + vec_y * steps_cos;
        container.push_back(offset_dx_dy(
                                v1,
                                offset,
                                offset * vec_x,
                                offset * vec_y
                            ));
    }

    // Now add the last point which is an offset from seg1.
    container.push_back(offset_point(v1, offset, seg1_unit_normal));
}

template <typename T, typename Container>
void offset_point_circle(geometry::point<T> const& v1, 
                         double offset,
                         double steps_per_radian,
                         double steps_cos,
                         double steps_sin,
                         Container & container) {
    std::size_t steps = static_cast<std::size_t>(std::round(steps_per_radian * 2.0 * M_PI));
    
    // Vector unit vector components to the position around the circle
    // starting at the unit normal vector of seg0.
    double vec_x = 1;
    double vec_y = 0;

    // Add the first position.
    container.push_back(offset_dx_dy(
                            v1, 
                            offset,
                            offset * vec_x,
                            offset * vec_y
                        ));
    
    // Finish out the circle
    for (std::size_t i = 1; i < steps; ++i) {
        // Calculate next vector position
        double prev_vec_x = vec_x;
        vec_x = prev_vec_x * steps_cos - vec_y * steps_sin;
        vec_y = prev_vec_x * steps_sin + vec_y * steps_cos;
        container.push_back(offset_dx_dy(
                                v1,
                                offset,
                                offset * vec_x,
                                offset * vec_y
                            ));
    }
}

}
}
}
