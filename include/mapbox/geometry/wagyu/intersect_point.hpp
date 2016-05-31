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
void IntersectPoint(edge<T> & Edge1, edge<T> & Edge2, mapbox::geometry::point<T> & ip)
{
    // This method finds the FIRST intersecting point in integer space between two edges
    // that is closest to the Bot point of the edges.
    using value_type = T;
    if (Edge1.Dx == Edge2.Dx)
    {
        ip.y = Edge1.Curr.y;
        ip.x = TopX(Edge1, ip.y);
        return;
    }
    else if (Edge1.Dx == 0.0)
    {
        ip.x = Edge1.Bot.x;
        if (IsHorizontal(Edge2))
        {
            ip.y = Edge2.Bot.y;
        }
        else
        {
            double b2 = Edge2.Bot.y - (Edge2.Bot.x / Edge2.Dx);
            if (Edge2.Bot.x == Edge1.Bot.x)
            {
                ip.y = std::round(ip.x / Edge2.Dx + b2);
            }
            else if (Edge2.Bot.x < Edge1.Bot.x)
            {
                ip.y = std::round((ip.x - 0.5) / Edge2.Dx + b2);
            }
            else
            {
                ip.y = std::round((ip.x + 0.5) / Edge2.Dx + b2);
            }
        }
    }
    else if (Edge2.Dx == 0.0)
    {
        ip.x = Edge2.Bot.x;
        if (IsHorizontal(Edge1))
        {
            ip.y = Edge1.Bot.y;
        }
        else
        {
            double b1 = Edge1.Bot.y - (Edge1.Bot.x / Edge1.Dx);
            if (Edge1.Bot.x == Edge2.Bot.x)
            {
                ip.y = Round(ip.x / Edge1.Dx + b1);
            }
            else if (Edge1.Bot.x < Edge2.Bot.x)
            {
                ip.y = Round((ip.x - 0.5) / Edge1.Dx + b1);
            }
            else
            {
                ip.y = Round((ip.x + 0.5) / Edge1.Dx + b1);
            }
        }
    } 
    else 
    {
        double b1 = Edge1.Bot.x - Edge1.Bot.y * Edge1.Dx;
        double b2 = Edge2.Bot.x - Edge2.Bot.y * Edge2.Dx;
        double q = (b2 - b1) / (Edge1.Dx - Edge2.Dx);
        ip.y = std::round(q);
        if (std::fabs(Edge1.Dx) < std::fabs(Edge2.Dx))
        {
            ip.x = std::round(Edge1.Dx * q + b1);
        }
        else 
        {
            ip.x = std::round(Edge2.Dx * q + b2);
        }
        // the idea is simply to looking closer
        // towards the origins of the lines (Edge1.Bot and Edge2.Bot)
        // until we do not find pixels that both lines travel through
        bool keep_searching = false;
        double by1 = Edge1.Bot.y - (Edge1.Bot.x / Edge1.Dx);
        double by2 = Edge2.Bot.y - (Edge2.Bot.x / Edge2.Dx);
        double bx1 = Edge1.Bot.x - (Edge1.Bot.y * Edge1.Dx);
        double bx2 = Edge2.Bot.x - (Edge2.Bot.y * Edge2.Dx);
        do
        {
            keep_searching = false;
            value_type y1 = nearest_along_y_dimension(Edge1.Bot.x,
                                                      Edge1.Bot.y,
                                                      Edge2.Bot.y,
                                                      ip.x,
                                                      ip.y,
                                                      by1,
                                                      Edge1.Dx);
            value_type y2 = nearest_along_y_dimension(Edge2.Bot.x,
                                                      Edge2.Bot.y,
                                                      Edge1.Bot.y,
                                                      ip.x,
                                                      ip.y,
                                                      by2,
                                                      Edge2.Dx);
            value_type x1 = nearest_along_x_dimension(Edge1.Bot.x,
                                                      Edge1.Bot.y,
                                                      Edge2.Bot.x,
                                                      ip.x,
                                                      ip.y,
                                                      bx1,
                                                      Edge1.Dx);
            value_type x2 = nearest_along_x_dimension(Edge2.Bot.x,
                                                      Edge2.Bot.y,
                                                      Edge1.Bot.x,
                                                      ip.x,
                                                      ip.y,
                                                      bx2,
                                                      Edge2.Dx);
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

    if (ip.y < Edge1.Top.y || ip.y < Edge2.Top.y) 
    {
        if (Edge1.Top.y > Edge2.Top.y)
        {
            ip.y = Edge1.Top.y;
        }
        else
        {
            ip.y = Edge2.Top.y;
        }
        if (std::fabs(Edge1.Dx) < std::fabs(Edge2.Dx))
        {
            ip.x = TopX(Edge1, ip.y);
        }
        else
        {
            ip.x = TopX(Edge2, ip.y);
        }
    } 
    //finally, don't allow 'ip' to be BELOW curr.y (ie bottom of scanbeam) ...
    if (ip.y > Edge1.Curr.y)
    {
        ip.y = Edge1.Curr.y;
        //use the more vertical edge to derive X ...
        if (std::fabs(Edge1.Dx) > std::fabs(Edge2.Dx))
        {
            ip.x = TopX(Edge2, ip.y);
        }
        else
        {
            ip.x = TopX(Edge1, ip.y);
        }
    }
}

}}}
