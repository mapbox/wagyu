#pragma once

#include <mapbox/geometry/wagyu/active_bound_list.hpp>
#include <mapbox/geometry/wagyu/bound.hpp>
#include <mapbox/geometry/wagyu/config.hpp>
#include <mapbox/geometry/wagyu/intersect.hpp>
#include <mapbox/geometry/wagyu/ring_util.hpp>
#include <mapbox/geometry/wagyu/sorted_bound_list.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox {
namespace geometry {
namespace wagyu {

template <typename T>
struct intersect_list_sorter {
    inline bool operator()(intersect_node<T> const& node1, intersect_node<T> const& node2) {
        if (!values_are_equal(node2.pt.y, node1.pt.y)) {
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

template <typename T1, typename T2>
bool get_edge_intersection(edge<T1> const& e1,
                           edge<T1> const& e2,
                           mapbox::geometry::point<T2>& pt) {
    T2 p0_x = static_cast<T2>(e1.bot.x);
    T2 p0_y = static_cast<T2>(e1.bot.y);
    T2 p1_x = static_cast<T2>(e1.top.x);
    T2 p1_y = static_cast<T2>(e1.top.y);
    T2 p2_x = static_cast<T2>(e2.bot.x);
    T2 p2_y = static_cast<T2>(e2.bot.y);
    T2 p3_x = static_cast<T2>(e2.top.x);
    T2 p3_y = static_cast<T2>(e2.top.y);
    T2 s1_x, s1_y, s2_x, s2_y;
    s1_x = p1_x - p0_x;
    s1_y = p1_y - p0_y;
    s2_x = p3_x - p2_x;
    s2_y = p3_y - p2_y;

    T2 s, t;
    s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0.0 && s <= 1.0 && t >= 0.0 && t <= 1.0) {
        pt.x = p0_x + (t * s1_x);
        pt.y = p0_y + (t * s1_y);
        return true;
    }
    return false;
}

template <typename T>
void add_extra_hot_pixels(T top_y,
                          local_minimum_ptr_list<T> const& minima_sorted,
                          local_minimum_ptr_list_itr<T> const& current_lm,
                          sorting_bound_list<T>& sorted_bound_list,
                          ring_manager<T>& rings) {
    active_bound_list<T> tmp_abl;
    auto lm = current_lm;
    while (lm != minima_sorted.end() && (*lm)->y == top_y) {
        if (!(*lm)->left_bound.edges.empty() && !(*lm)->right_bound.edges.empty()) {
            mapbox::geometry::point<T> hp((*lm)->left_bound.edges.front().bot.x, top_y);
            add_to_hot_pixels(hp, rings);
        }
        auto& left_bound = (*lm)->left_bound;
        left_bound.current_edge = left_bound.edges.begin();
        left_bound.curr.x = static_cast<double>(left_bound.current_edge->bot.x);
        left_bound.curr.y = static_cast<double>(left_bound.current_edge->bot.y);
        if (!left_bound.edges.empty()) {
            insert_bound_into_SBL(left_bound, tmp_abl, sorted_bound_list);
        }
        auto& right_bound = (*lm)->right_bound;
        right_bound.current_edge = right_bound.edges.begin();
        right_bound.curr.x = static_cast<double>(right_bound.current_edge->bot.x);
        right_bound.curr.y = static_cast<double>(right_bound.current_edge->bot.y);
        if (!right_bound.edges.empty()) {
            insert_bound_into_SBL(right_bound, tmp_abl, sorted_bound_list);
        }
        ++lm;
    }

    // We now need to add hot pixels for intersections that might occur -0.5 below the top_y
    for (auto bnd_itr = sorted_bound_list.begin(); bnd_itr != sorted_bound_list.end();) {
        bound_ptr<T> bnd = *(bnd_itr->bound);
        if (bnd->current_edge->top.y == top_y) {
            // Add a hot pixel at the top
            add_to_hot_pixels(bnd->current_edge->top, rings);
            // Go to another edge or delete bnd_itr
            auto edge_itr = bnd->current_edge;
            while (edge_itr != bnd->edges.end() && edge_itr->top.y == top_y) {
                if (is_horizontal(*edge_itr)) {
                    T starting_x = edge_itr->bot.x;
                    T ending_x = edge_itr->top.x;
                    T min_x = std::min(starting_x, ending_x);
                    T max_x = std::max(starting_x, ending_x);
                    for (auto bnd_itr2 = sorted_bound_list.begin();
                         bnd_itr2 != sorted_bound_list.end(); ++bnd_itr2) {
                        bound_ptr<T> bnd2 = *(bnd_itr2->bound);
                        if (bnd == bnd2 || bnd_itr2->current_x < min_x ||
                            bnd_itr2->current_x > max_x || bnd2->current_edge->top.y == top_y ||
                            bnd2->current_edge->bot.y == top_y) {
                            continue;
                        }
                        mapbox::geometry::point<T> hp(std::llround(bnd_itr2->current_x), top_y);
                        add_to_hot_pixels(hp, rings);
                    }
                }
                ++edge_itr;
            }
            if (edge_itr == bnd->edges.end()) {
                bnd_itr = sorted_bound_list.erase(bnd_itr);
                continue;
            }
            bnd_itr->current_edge = &(*edge_itr);
            bnd_itr->current_x = bnd_itr->current_edge->bot.x;
            mapbox::geometry::point<T> hp(bnd_itr->current_edge->bot.x, top_y);
            add_to_hot_pixels(hp, rings);
        }
        ++bnd_itr;
    }

    sorted_bound_list.sort(sorting_bound_current_sorter<T>());

    for (auto bnd_itr = sorted_bound_list.begin(); bnd_itr != sorted_bound_list.end(); ++bnd_itr) {
        bnd_itr->current_x = get_current_x(*(bnd_itr->current_edge), top_y - 1);
    }

    // bubblesort ...
    bool isModified;
    do {
        isModified = false;
        auto bnd = sorted_bound_list.begin();
        auto bnd_next = std::next(bnd);
        while (bnd_next != sorted_bound_list.end()) {
            if (bnd->current_x > bnd_next->current_x) {
                mapbox::geometry::point<double> pt;
                if (get_edge_intersection<T, double>(*(bnd->current_edge),
                                                     *(bnd_next->current_edge), pt)) {
                    mapbox::geometry::point<T> hp(std::llround(pt.x), std::llround(pt.y));
                    if (hp.y >= top_y) {
                        add_to_hot_pixels(hp, rings);
                    }
                }
                swap_positions_in_SBL(bnd, bnd_next, sorted_bound_list);
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
void build_intersect_list(sorting_bound_list<T>& sorted_bound_list,
                          intersect_list<T>& intersects,
                          ring_manager<T>& rings) {
    // bubblesort ...
    bool isModified;
    do {
        isModified = false;
        auto bnd = sorted_bound_list.begin();
        auto bnd_next = std::next(bnd);
        while (bnd_next != sorted_bound_list.end()) {
            if (bnd->current_x > bnd_next->current_x &&
                !slopes_equal(*(bnd->current_edge), *(bnd_next->current_edge))) {
                mapbox::geometry::point<double> pt;
                if (!get_edge_intersection<T, double>(*(bnd->current_edge),
                                                      *(bnd_next->current_edge), pt)) {
                    throw std::runtime_error(
                        "Trying to find intersection of lines that do not intersect");
                }
                intersects.emplace_back(bnd, bnd_next, pt);
                mapbox::geometry::point<T> hp(std::llround(pt.x), std::llround(pt.y));
                add_to_hot_pixels(hp, rings);
                swap_positions_in_SBL(bnd, bnd_next, sorted_bound_list);
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
    sorted_bound_list.sort(sorting_bound_index_sorter<T>());

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
        swap_positions_in_SBL(intersects[i].bound1, intersects[i].bound2, sorted_bound_list);
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
                      ring_manager<T>& rings,
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

    fill_type b1FillType, b2FillType, b1FillType2, b2FillType2;
    if ((*b1)->poly_type == polygon_type_subject) {
        b1FillType = subject_fill_type;
        b1FillType2 = clip_fill_type;
    } else {
        b1FillType = clip_fill_type;
        b1FillType2 = subject_fill_type;
    }
    if ((*b2)->poly_type == polygon_type_subject) {
        b2FillType = subject_fill_type;
        b2FillType2 = clip_fill_type;
    } else {
        b2FillType = clip_fill_type;
        b2FillType2 = subject_fill_type;
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
            (*b2)->last_point = pt;
            swap_sides(*(*b1), *(*b2));
            swap_rings(*(*b1), *(*b2));
        }
    } else if (b2Contributing) {
        if (b1Wc == 0 || b1Wc == 1) {
            (*b1)->last_point = pt;
            add_point(b2, active_bounds, pt, rings);
            swap_sides(*(*b1), *(*b2));
            swap_rings(*(*b1), *(*b2));
        }
    } else if ((b1Wc == 0 || b1Wc == 1) && (b2Wc == 0 || b2Wc == 1)) {
        // neither bound is currently contributing ...

        std::int32_t b1Wc2, b2Wc2;
        switch (b1FillType2) {
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
        switch (b2FillType2) {
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
            add_local_minimum_point(b1, b2, active_bounds, pt, rings);
        } else if (b1Wc == 1 && b2Wc == 1) {
            switch (cliptype) {
            case clip_type_intersection:
                if (b1Wc2 > 0 && b2Wc2 > 0) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings);
                }
                break;
            default:
            case clip_type_union:
                if (b1Wc2 <= 0 && b2Wc2 <= 0) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings);
                }
                break;
            case clip_type_difference:
                if ((((*b1)->poly_type == polygon_type_clip) && (b1Wc2 > 0) && (b2Wc2 > 0)) ||
                    (((*b1)->poly_type == polygon_type_subject) && (b1Wc2 <= 0) && (b2Wc2 <= 0))) {
                    add_local_minimum_point(b1, b2, active_bounds, pt, rings);
                }
                break;
            case clip_type_x_or:
                add_local_minimum_point(b1, b2, active_bounds, pt, rings);
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
                            ring_manager<T>& rings,
                            active_bound_list<T>& active_bounds) {
    for (auto& node : intersects) {
        mapbox::geometry::point<T> pt(std::llround(node.pt.x), std::llround(node.pt.y));
        intersect_bounds(node.bound1->bound, node.bound2->bound, pt, cliptype, subject_fill_type,
                         clip_fill_type, rings, active_bounds);
        swap_positions_in_ABL(node.bound1->bound, node.bound2->bound, active_bounds);
    }
}

template <typename T>
void process_intersections(T top_y,
                           local_minimum_ptr_list<T> const& minima_sorted,
                           local_minimum_ptr_list_itr<T> const& current_lm,
                           active_bound_list<T>& active_bounds,
                           clip_type cliptype,
                           fill_type subject_fill_type,
                           fill_type clip_fill_type,
                           ring_manager<T>& rings) {
    sorting_bound_list<T> sorted_bound_list;
    std::size_t index = 0;
    // create sorted edge list from AEL
    for (auto bnd_itr = active_bounds.begin(); bnd_itr != active_bounds.end(); ++bnd_itr) {
        double curr_x = get_current_x(*((*bnd_itr)->current_edge), top_y);
        (*bnd_itr)->curr.x = curr_x;
        sorted_bound_list.emplace_back(bnd_itr, &(*((*bnd_itr)->current_edge)), index, curr_x);
        ++index;
    }
    intersect_list<T> intersects;
    build_intersect_list(sorted_bound_list, intersects, rings);
    if (intersects.empty()) {
        add_extra_hot_pixels(top_y, minima_sorted, current_lm, sorted_bound_list, rings);
        return;
    }
    sorting_bound_list<T> sorted_bound_list2 = sorted_bound_list;
    if (intersects.size() > 1) {
        fixup_intersection_order(sorted_bound_list, intersects);
    }
    add_extra_hot_pixels(top_y, minima_sorted, current_lm, sorted_bound_list2, rings);

    process_intersect_list(intersects, cliptype, subject_fill_type, clip_fill_type, rings,
                           active_bounds);
}
}
}
}
