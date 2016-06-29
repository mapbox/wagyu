#pragma once

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/edge_util.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/intersect_point.hpp>
#include <mapbox/geometry/wagyu/join.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/sorted_bound_list.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct intersect_list_sorter {
    inline bool operator()(intersect_node<T> const& node1, intersect_node<T> const& node2) {
        if (node2.pt.y != node1.pt.y) {
            return node2.pt.y < node1.pt.y;
        } else {
            return ((*node2.bound1->bound)->winding_count2 +
                    (*node2.bound2->bound)->winding_count2) >
                   ((*node1.bound1->bound)->winding_count2 +
                    (*node1.bound2->bound)->winding_count2);
        }
    }
};

template <typename T>
inline void swap_rings(bound<T>& b1, bound<T>& b2) {
    ring_ptr<T> ring = b1.ring;
    b1.ring = b2.ring;
    b2.ring = ring;
}

template <typename T>
inline void swap_sides(bound<T>& b1, bound<T>& b2) {
    edge_side side = b1.side;
    b1.side = b2.side;
    b2.side = side;
}

template <typename T>
void build_intersect_list(T top_y,
                          sorting_bound_list<T>& sorted_bound_list,
                          intersect_list<T>& intersects) {
    // bubblesort ...
    bool isModified;
    do {
        isModified = false;
        auto bnd = sorted_bound_list.begin();
        auto bnd_next = std::next(bnd);
        while (bnd_next != sorted_bound_list.end()) {
            mapbox::geometry::point<T> pt;
            if ((*(bnd->bound))->curr.x > (*(bnd_next->bound))->curr.x) {
                intersection_point(*(*(bnd->bound)), *(*(bnd_next->bound)), pt);
                if (pt.y < top_y) {
                    pt = mapbox::geometry::point<T>(
                        get_current_x(*((*(bnd->bound))->current_edge), top_y), top_y);
                }
                intersects.emplace_back(bnd, bnd_next, pt);
                sorted_bound_list.splice(bnd, sorted_bound_list, bnd_next);
                bnd_next = std::next(bnd);
                isModified = true;
            } else {
                bnd = bnd_next;
                ++bnd_next;
            }
        }
    } while (isModified);
}

template <typename T>
bool bounds_adjacent(intersect_node<T> const& inode) {
    return (std::next(inode.bound1) == inode.bound2) || (std::next(inode.bound2) == inode.bound1);
}

template <typename T>
void fixup_intersection_order(sorting_bound_list<T>& sorted_bound_list,
                              intersect_list<T>& intersects) {
    // Precondition: Intersections are sorted bottom-most first.
    // It's crucial that intersections are only made between adjacent bounds,
    // so reorder the intersections to ensure this if necessary.

    // resort sorted bound list to the same as the active bound list
    sorted_bound_list.sort(sorting_bound_sorter<T>());

    // Sort the intersection list
    std::stable_sort(intersects.begin(), intersects.end(), intersect_list_sorter<T>());

    std::size_t n = intersects.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!bounds_adjacent(intersects[i])) {
            std::size_t j = i + 1;
            while (j < n && !bounds_adjacent(intersects[j])) {
                j++;
            }
            if (j == n) {
                // Intersection could not be made adjacent
                throw clipper_exception("Unable to fixup intersection order!");
            }
            std::swap(intersects[i], intersects[j]);
        }
        sorted_bound_list.splice(intersects[i].bound1, sorted_bound_list, intersects[i].bound2);
    }
    return;
}

template <typename T>
void intersect_bounds(active_bound_list_itr<T>& b1,
                      active_bound_list_itr<T>& b2,
                      mapbox::geometry::point<T> const& pt,
                      clip_type cliptype,
                      fill_type subject_fill_type,
                      fill_type clip_fill_type,
                      ring_list<T>& rings,
                      join_list<T>& joins,
                      active_bound_list<T>& active_bounds) {
    bool b1Contributing = ((*b1)->ring != nullptr);
    bool b2Contributing = ((*b2)->ring != nullptr);

    // if either bound is on an OPEN path ...
    if ((*b1)->winding_delta == 0 || (*b2)->winding_delta == 0) {
        // ignore subject-subject open path intersections UNLESS they
        // are both open paths, AND they are both 'contributing maximas' ...
        if ((*b1)->winding_delta == 0 && (*b2)->winding_delta == 0) {
            return;
        }

        // if intersecting a subj line with a subj poly ...
        else if ((*b1)->poly_type == (*b2)->poly_type &&
                 (*b1)->winding_delta != (*b2)->winding_delta && cliptype == clip_type_union) {
            if ((*b1)->winding_delta == 0) {
                if (b2Contributing) {
                    add_point(b1, active_bounds, pt, rings);
                    if (b1Contributing) {
                        (*b1)->ring = nullptr;
                    }
                }
            } else {
                if (b1Contributing) {
                    add_point(b2, active_bounds, pt, rings);
                    if (b2Contributing) {
                        (*b2)->ring = nullptr;
                    }
                }
            }
        } else if ((*b1)->poly_type != (*b2)->poly_type) {
            // toggle subj open path index on/off when std::abs(clip.WndCnt) == 1
            if (((*b1)->winding_delta == 0) && std::abs((*b2)->winding_count) == 1 &&
                (cliptype != clip_type_union || (*b2)->winding_count2 == 0)) {
                add_point(b1, active_bounds, pt, rings);
                if (b1Contributing) {
                    (*b1)->ring = nullptr;
                }
            } else if (((*b2)->winding_delta == 0) && (std::abs((*b1)->winding_count) == 1) &&
                       (cliptype != clip_type_union || (*b1)->winding_count2 == 0)) {
                add_point(b2, active_bounds, pt, rings);
                if (b2Contributing) {
                    (*b2)->ring = nullptr;
                }
            }
        }
        return;
    }

    // update winding counts...
    // assumes that b1 will be to the Right of b2 ABOVE the intersection
    if ((*b1)->poly_type == (*b2)->poly_type) {
        if (is_even_odd_fill_type(*(*b1), subject_fill_type, clip_fill_type)) {
            std::int32_t oldE1winding_count = (*b1)->winding_count;
            (*b1)->winding_count = (*b2)->winding_count;
            (*b2)->winding_count = oldE1winding_count;
        } else {
            if ((*b1)->winding_count + (*b2)->winding_delta == 0) {
                (*b1)->winding_count = -(*b1)->winding_count;
            } else {
                (*b1)->winding_count += (*b2)->winding_delta;
            }
            if ((*b2)->winding_count - (*b1)->winding_delta == 0) {
                (*b2)->winding_count = -(*b2)->winding_count;
            } else {
                (*b2)->winding_count -= (*b1)->winding_delta;
            }
        }
    } else {
        if (!is_even_odd_fill_type(*(*b2), subject_fill_type, clip_fill_type)) {
            (*b1)->winding_count2 += (*b2)->winding_delta;
        } else {
            (*b1)->winding_count2 = ((*b1)->winding_count2 == 0) ? 1 : 0;
        }
        if (!is_even_odd_fill_type(*(*b1), subject_fill_type, clip_fill_type)) {
            (*b2)->winding_count2 -= (*b1)->winding_delta;
        } else {
            (*b2)->winding_count2 = ((*b2)->winding_count2 == 0) ? 1 : 0;
        }
    }

    fill_type b1FillType, b2FillType, b1FillTypb2, b2FillTypb2;
    if ((*b1)->poly_type == polygon_type_subject) {
        b1FillType = subject_fill_type;
        b1FillTypb2 = clip_fill_type;
    } else {
        b1FillType = clip_fill_type;
        b1FillTypb2 = subject_fill_type;
    }
    if ((*b2)->poly_type == polygon_type_subject) {
        b2FillType = subject_fill_type;
        b2FillTypb2 = clip_fill_type;
    } else {
        b2FillType = clip_fill_type;
        b2FillTypb2 = subject_fill_type;
    }

    std::int32_t b1Wc, b2Wc;
    switch (b1FillType) {
    case fill_type_positive:
        b1Wc = (*b1)->winding_count;
        break;
    case fill_type_negative:
        b1Wc = -(*b1)->winding_count;
        break;
    case fill_type_even_odd:
    case fill_type_non_zero:
    default:
        b1Wc = std::abs((*b1)->winding_count);
    }
    switch (b2FillType) {
    case fill_type_positive:
        b2Wc = (*b2)->winding_count;
        break;
    case fill_type_negative:
        b2Wc = -(*b2)->winding_count;
        break;
    case fill_type_even_odd:
    case fill_type_non_zero:
    default:
        b2Wc = std::abs((*b2)->winding_count);
    }

    if (b1Contributing && b2Contributing) {
        if ((b1Wc != 0 && b1Wc != 1) || (b2Wc != 0 && b2Wc != 1) ||
            ((*b1)->poly_type != (*b2)->poly_type && cliptype != clip_type_x_or)) {
            add_local_maximum_point(b1, b2, pt, rings, active_bounds);
        } else {
            add_point(b1, active_bounds, pt, rings);
            add_point(b2, active_bounds, pt, rings);
            swap_sides(*(*b1), *(*b2));
            swap_rings(*(*b1), *(*b2));
        }
    } else if (b1Contributing) {
        if (b2Wc == 0 || b2Wc == 1) {
            add_point(b1, active_bounds, pt, rings);
            swap_sides(*(*b1), *(*b2));
            swap_rings(*(*b1), *(*b2));
        }
    } else if (b2Contributing) {
        if (b1Wc == 0 || b1Wc == 1) {
            add_point(b2, active_bounds, pt, rings);
            swap_sides(*(*b1), *(*b2));
            swap_rings(*(*b1), *(*b2));
        }
    } else if ((b1Wc == 0 || b1Wc == 1) && (b2Wc == 0 || b2Wc == 1)) {
        // neither bound is currently contributing ...

        std::int32_t b1Wc2, b2Wc2;
        switch (b1FillTypb2) {
        case fill_type_positive:
            b1Wc2 = (*b1)->winding_count2;
            break;
        case fill_type_negative:
            b1Wc2 = -(*b1)->winding_count2;
            break;
        case fill_type_even_odd:
        case fill_type_non_zero:
        default:
            b1Wc2 = std::abs((*b1)->winding_count2);
        }
        switch (b2FillTypb2) {
        case fill_type_positive:
            b2Wc2 = (*b2)->winding_count2;
            break;
        case fill_type_negative:
            b2Wc2 = -(*b2)->winding_count2;
            break;
        case fill_type_even_odd:
        case fill_type_non_zero:
        default:
            b2Wc2 = std::abs((*b2)->winding_count2);
        }

        if ((*b1)->poly_type != (*b2)->poly_type) {
            add_local_minimum_point(b1, b2, active_bounds, pt, rings, joins);
        } else if (b1Wc == 1 && b2Wc == 1) {
            switch (cliptype) {
            case clip_type_intersection:
                if (b1Wc2 > 0 && b2Wc2 > 0) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings, joins);
                }
                break;
            default:
            case clip_type_union:
                if (b1Wc2 <= 0 && b2Wc2 <= 0) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings, joins);
                }
                break;
            case clip_type_difference:
                if ((((*b1)->poly_type == polygon_type_clip) && (b1Wc2 > 0) && (b2Wc2 > 0)) ||
                    (((*b1)->poly_type == polygon_type_subject) && (b1Wc2 <= 0) && (b2Wc2 <= 0))) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings, joins);
                }
                break;
            case clip_type_x_or:
                add_local_minimum_point(b1, b2, active_bounds, pt, rings, joins);
            }
        } else {
            swap_sides(*(*b1), *(*b2));
        }
    }
}

template <typename T>
void process_intersect_list(intersect_list<T>& intersects,
                            clip_type cliptype,
                            fill_type subject_fill_type,
                            fill_type clip_fill_type,
                            ring_list<T>& rings,
                            join_list<T>& joins,
                            active_bound_list<T>& active_bounds) {
    for (auto& node : intersects) {
        intersect_bounds(node.bound1->bound, node.bound2->bound, node.pt, cliptype, subject_fill_type,
                         clip_fill_type, rings, joins, active_bounds);
        active_bounds.splice(node.bound1->bound, active_bounds, node.bound2->bound);
    }
}

template <typename T>
void process_intersections(T top_y,
                           active_bound_list<T>& active_bounds,
                           clip_type cliptype,
                           fill_type subject_fill_type,
                           fill_type clip_fill_type,
                           ring_list<T>& rings,
                           join_list<T>& joins) {
    if (!active_bounds.empty()) {
        return;
    }
    sorting_bound_list<T> sorted_bound_list;
    std::size_t index = 0;
    // create sorted edge list from AEL
    for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
        (*bnd_itr)->curr.x = get_current_x(*((*bnd_itr)->current_edge), top_y);
        sorted_bound_list.emplace_back(bnd_itr, index);
        ++index;
    }

    intersect_list<T> intersects;
    build_intersect_list(top_y, sorted_bound_list, intersects);

    if (intersects.empty()) {
        return;
    }
    if (intersects.size() > 1) {
        fixup_intersection_order(sorted_bound_list, intersects);
    }
    process_intersect_list(intersects, cliptype, subject_fill_type, clip_fill_type, rings, joins,
                           active_bounds);
    return;
}
}
}
}
