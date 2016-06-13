#pragma once

#include <mapbox/geometry/wagyu/edge.hpp>
#include <mapbox/geometry/wagyu/util.hpp>

namespace mapbox { namespace geometry { namespace wagyu {

template <typename T>
inline T nearest_along_y_dimension(T const edge_bot_x, 
                                   T const edge_bot_y,
                                   T const edge_2_bot_y,
                                   T const ip_x,
                                   T const ip_y,
                                   double const by,
                                   double const edge_dx)
{
    using value_type = T;
    value_type result = ip_y;
    if (edge_bot_x > ip_x)
    {
        if (edge_bot_y >= ip_y)
        {
            result = std::floor(((ip_x + 0.5) / edge_dx + by) + 0.5);
        }
        else
        {
            result = std::ceil(((ip_x + 0.5) / edge_dx + by) - 0.5);
        }
    }
    else if (edge_bot_x < ip_x)
    {
        if (edge_bot_y >= ip_y)
        {
            result = std::floor(((ip_x - 0.5) / edge_dx + by) + 0.5);
        }
        else
        {
            result = std::ceil(((ip_x - 0.5) / edge_dx + by) - 0.5);
        }
    }
    else if (edge_bot_y > ip_y)
    {
        if (edge_2_bot_y >= edge_bot_y)
        {
            result = edge_bot_y;
        }
        else
        {
            result = edge_2_bot_y;
        }
    }
    else if (edge_bot_y < ip_y)
    {
        if (edge_2_bot_y <= edge_bot_y)
        {
            result = edge_bot_y;
        }
        else
        {
            result = edge_2_bot_y;
        }
    }
    if (ip_y >= edge_bot_y && result < edge_bot_y)
    {
        result = edge_bot_y;
    }
    else if (ip_y <= edge_bot_y && result > edge_bot_y)
    {
        result = edge_bot_y;
    }
    return result;
}

template <typename T>
inline T nearest_along_x_dimension(T const edge_bot_x, 
                                   T const edge_bot_y,
                                   T const edge_2_bot_x,
                                   T const ip_x,
                                   T const ip_y,
                                   double const bx,
                                   double const edge_dx)
{
    using value_type = T;
    value_type result = ip_x;
    if (edge_bot_y > ip_y)
    {
        if (edge_bot_x >= ip_x)
        {
            result = std::floor(((ip_y + 0.5) * edge_dx + bx) + 0.5);
        }
        else
        {
            result = std::ceil(((ip_y + 0.5) * edge_dx + bx) - 0.5);
        }
    }
    else if (edge_bot_y < ip_y)
    {
        if (edge_bot_x >= ip_x)
        {
            result = std::floor(((ip_y - 0.5) * edge_dx + bx) + 0.5);
        }
        else
        {
            result = std::ceil(((ip_y - 0.5) * edge_dx + bx) - 0.5);
        }
    }
    else if (edge_bot_x > ip_x)
    {
        if (edge_2_bot_x >= edge_bot_x)
        {
            result = edge_bot_x;
        }
        else
        {
            result = edge_2_bot_x;
        }
    }
    else if (edge_bot_x < ip_x)
    {
        if (edge_2_bot_x <= edge_bot_x)
        {
            result = edge_bot_x;
        }
        else
        {
            result = edge_2_bot_x;
        }
    }
    if (ip_x >= edge_bot_x && result < edge_bot_x)
    {
        result = edge_bot_x;
    }
    else if (ip_x <= edge_bot_x && result > edge_bot_x)
    {
        result = edge_bot_x;
    }
    return result;
}

template <typename T>
void intersection_point(edge<T> const& Edge1, 
                        edge<T> const& Edge2, 
                        mapbox::geometry::point<T> & ip)
{
    // This method finds the FIRST intersecting point in integer space between two edges
    // that is closest to the bot point of the edges.
    using value_type = T;
    if (Edge1.dx == Edge2.dx)
    {
        ip.y = Edge1.curr.y;
        ip.x = get_current_x(Edge1, ip.y);
        return;
    }
    else if (Edge1.dx == 0.0)
    {
        ip.x = Edge1.bot.x;
        if (is_horizontal(Edge2))
        {
            ip.y = Edge2.bot.y;
        }
        else
        {
            double b2 = Edge2.bot.y - (Edge2.bot.x / Edge2.dx);
            if (Edge2.bot.x == Edge1.bot.x)
            {
                ip.y = std::round(ip.x / Edge2.dx + b2);
            }
            else if (Edge2.bot.x < Edge1.bot.x)
            {
                ip.y = std::round((ip.x - 0.5) / Edge2.dx + b2);
            }
            else
            {
                ip.y = std::round((ip.x + 0.5) / Edge2.dx + b2);
            }
        }
    }
    else if (Edge2.dx == 0.0)
    {
        ip.x = Edge2.bot.x;
        if (is_horizontal(Edge1))
        {
            ip.y = Edge1.bot.y;
        }
        else
        {
            double b1 = Edge1.bot.y - (Edge1.bot.x / Edge1.dx);
            if (Edge1.bot.x == Edge2.bot.x)
            {
                ip.y = std::round(ip.x / Edge1.dx + b1);
            }
            else if (Edge1.bot.x < Edge2.bot.x)
            {
                ip.y = std::round((ip.x - 0.5) / Edge1.dx + b1);
            }
            else
            {
                ip.y = std::round((ip.x + 0.5) / Edge1.dx + b1);
            }
        }
    } 
    else 
    {
        double b1 = Edge1.bot.x - Edge1.bot.y * Edge1.dx;
        double b2 = Edge2.bot.x - Edge2.bot.y * Edge2.dx;
        double q = (b2 - b1) / (Edge1.dx - Edge2.dx);
        ip.y = std::round(q);
        if (std::fabs(Edge1.dx) < std::fabs(Edge2.dx))
        {
            ip.x = std::round(Edge1.dx * q + b1);
        }
        else 
        {
            ip.x = std::round(Edge2.dx * q + b2);
        }
        // the idea is simply to looking closer
        // towards the origins of the lines (Edge1.bot and Edge2.bot)
        // until we do not find pixels that both lines travel through
        bool keep_searching = false;
        double by1 = Edge1.bot.y - (Edge1.bot.x / Edge1.dx);
        double by2 = Edge2.bot.y - (Edge2.bot.x / Edge2.dx);
        double bx1 = Edge1.bot.x - (Edge1.bot.y * Edge1.dx);
        double bx2 = Edge2.bot.x - (Edge2.bot.y * Edge2.dx);
        do
        {
            keep_searching = false;
            value_type y1 = nearest_along_y_dimension(Edge1.bot.x,
                                                      Edge1.bot.y,
                                                      Edge2.bot.y,
                                                      ip.x,
                                                      ip.y,
                                                      by1,
                                                      Edge1.dx);
            value_type y2 = nearest_along_y_dimension(Edge2.bot.x,
                                                      Edge2.bot.y,
                                                      Edge1.bot.y,
                                                      ip.x,
                                                      ip.y,
                                                      by2,
                                                      Edge2.dx);
            value_type x1 = nearest_along_x_dimension(Edge1.bot.x,
                                                      Edge1.bot.y,
                                                      Edge2.bot.x,
                                                      ip.x,
                                                      ip.y,
                                                      bx1,
                                                      Edge1.dx);
            value_type x2 = nearest_along_x_dimension(Edge2.bot.x,
                                                      Edge2.bot.y,
                                                      Edge1.bot.x,
                                                      ip.x,
                                                      ip.y,
                                                      bx2,
                                                      Edge2.dx);
            if (y1 > ip.y && y2 > ip.y)
            {
                ip.y = std::min(y1,y2);
                keep_searching = true;
            }
            else if (y1 < ip.y && y2 < ip.y)
            {
                ip.y = std::max(y1,y2);
                keep_searching = true;
            } 
            if (x1 > ip.x && x2 > ip.x)
            {
                ip.x = std::min(x1,x2);
                keep_searching = true;
            }
            else if (x1 < ip.x && x2 < ip.x)
            {
                ip.x = std::max(x1,x2);
                keep_searching = true;
            }
        }
        while (keep_searching);
    }

    if (ip.y < Edge1.top.y || ip.y < Edge2.top.y) 
    {
        if (Edge1.top.y > Edge2.top.y)
        {
            ip.y = Edge1.top.y;
        }
        else
        {
            ip.y = Edge2.top.y;
        }
        if (std::fabs(Edge1.dx) < std::fabs(Edge2.dx))
        {
            ip.x = get_current_x(Edge1, ip.y);
        }
        else
        {
            ip.x = get_current_x(Edge2, ip.y);
        }
    } 
    //finally, don't allow 'ip' to be BELOW curr.y (ie bottom of scanbeam) ...
    if (ip.y > Edge1.curr.y)
    {
        ip.y = Edge1.curr.y;
        //use the more vertical edge to derive X ...
        if (std::fabs(Edge1.dx) > std::fabs(Edge2.dx))
        {
            ip.x = get_current_x(Edge2, ip.y);
        }
        else
        {
            ip.x = get_current_x(Edge1, ip.y);
        }
    }
}

}}}
