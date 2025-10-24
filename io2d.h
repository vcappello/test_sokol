#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_glue.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <cmath>
#include <numbers>
#include <limits>

namespace io2d
{

    class rgba_color
    {
    public:
        using channel_t = float;

        rgba_color() {}
        rgba_color(channel_t r, channel_t g, channel_t b, channel_t a = 1.0f) : r(r), g(g), b(b), a(a) {}
        rgba_color(uint32_t argb)
        {
            a = ((argb >> 24) & 0xff) / 255.0f;
            r = ((argb >> 16) & 0xff) / 255.0f;
            g = ((argb >> 8) & 0xff) / 255.0f;
            b = (argb & 0xff) / 255.0f;
        }

    public:
        channel_t r = 0.0f;
        channel_t g = 0.0f;
        channel_t b = 0.0f;
        channel_t a = 1.0f;
    };

    /**
     * @brief Stroke style for path drawing
     */
    class stroke_style_s
    {
    public:
        stroke_style_s() {}
        stroke_style_s(const rgba_color &color) : color(color) {}

    public:
        rgba_color color;
        float width = 1.0f;
    };

    /**
     * @brief Fill style for path drawing
     */
    class fill_style_s
    {
    public:
        rgba_color color;
    };

    /**
     * @brief Base class for path elements
     */
    class abstract_sub_path
    {
    public:
        abstract_sub_path() {}
        virtual ~abstract_sub_path() {}

        virtual void stroke(const stroke_style_s &style) = 0;
        virtual void fill(const fill_style_s &style) = 0;
    };

    /**
     * @brief A line path element
     */
    class path_line : public abstract_sub_path
    {
    public:
        path_line(const sgp_point &pt1, const sgp_point &pt2) : _pt1(pt1),
                                                                _pt2(pt2)
        {
        }
        virtual ~path_line() {}

        /**
         * @brief Draw the line using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);
            if (style.width == 1.0f)
            {
                sgp_draw_line(_pt1.x, _pt1.y, _pt2.x, _pt2.y);
            }
            else
            {
                draw_thik_line(_pt1, _pt2, style.width);
            }
        }

        /**
         * @brief Fill does nothing for line
         */
        void fill(const fill_style_s &style) override
        {
        }

        /**
         * @brief Get an array of 4 points for draw a thick line using triangle strip
         *
         * 0  S   1
         * +--+--+
         * |    /|
         * |   / |
         * |  /  |
         * | /   |
         * |/    |
         * +--+--+
         * 3  E   2
         *
         * To draw a line with thickness starting from S end ending to E we need to create
         * a rectangle using 2 triangles 0-1-3 and 1-3-2. This means the triangle strip must be
         * drawn using the points in the order 0, 1, 3, 2
         *
         * @param start The start point
         * @param end The end point
         * @param thickness The line thickness
         * @return std::vector<sgp_point> The array of points
         */
        static std::vector<sgp_point> get_thick_line_points(sgp_point start, sgp_point end, float thickness)
        {
            GLfloat d = std::sqrt((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y));
            GLfloat y_shift = thickness * (end.x - start.x) / (d * 2.0f);
            GLfloat x_shift = -thickness * (end.y - start.y) / (d * 2.0f);

            return std::vector<sgp_point>{
                sgp_point{start.x - x_shift, start.y - y_shift},
                sgp_point{start.x + x_shift, start.y + y_shift},
                sgp_point{end.x + x_shift, end.y + y_shift},
                sgp_point{end.x - x_shift, end.y - y_shift}};
        }

        /**
         * @brief Draw a thick line using triangle strip
         */
        static void draw_thik_line(sgp_point start, sgp_point end, float thickness)
        {
            auto line_points = get_thick_line_points(start, end, thickness);

            std::array<sgp_point, 4> points = {line_points[0],
                                               line_points[1],
                                               line_points[3],
                                               line_points[2]};

            sgp_draw_filled_triangles_strip(points.data(), points.size());
        }

        static void draw_thik_lines(const std::vector<sgp_point> &points, float thickness)
        {
            if (points.size() < 2)
                return;

            for (int i = 1; i < points.size(); i++)
            {
                draw_thik_line(points[i - 1], points[i], thickness);
            }
        }

    protected:
        sgp_point _pt1;
        sgp_point _pt2;
    };

    /**
     * @brief A rectangle path element
     */
    class path_rect : public abstract_sub_path
    {
    public:
        path_rect(const sgp_point &pt1, const sgp_point &pt2) : _pt1(pt1),
                                                                _pt2(pt2)
        {
        }
        virtual ~path_rect() {}

        /**
         * @brief Draw the rectangle using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);
            if (style.width == 1.0f)
            {
                std::array<sgp_point, 5> points = {sgp_point{_pt1.x, _pt1.y},
                                                   sgp_point{_pt2.x, _pt1.y},
                                                   sgp_point{_pt2.x, _pt2.y},
                                                   sgp_point{_pt1.x, _pt2.y},
                                                   sgp_point{_pt1.x, _pt1.y}};
                sgp_draw_lines_strip(points.data(), points.size());
            }
            else
            {
                path_line::draw_thik_line(sgp_point{_pt1.x, _pt1.y}, sgp_point{_pt2.x, _pt1.y}, style.width);
                path_line::draw_thik_line(sgp_point{_pt2.x, _pt1.y}, sgp_point{_pt2.x, _pt2.y}, style.width);
                path_line::draw_thik_line(sgp_point{_pt2.x, _pt2.y}, sgp_point{_pt1.x, _pt2.y}, style.width);
                path_line::draw_thik_line(sgp_point{_pt1.x, _pt2.y}, sgp_point{_pt1.x, _pt1.y}, style.width);
            }
        }

        /**
         * @brief Fill the rectangle using the fill style
         */
        void fill(const fill_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);
            sgp_draw_filled_rect(_pt1.x, _pt1.y, _pt2.x - _pt1.x, _pt2.y - _pt1.y);
        }

    protected:
        sgp_point _pt1;
        sgp_point _pt2;
    };

    /**
     * @brief An ellipse path element
     */
    class path_ellipse : public abstract_sub_path
    {
    public:
        struct ellipse_data
        {
            float cx; // Center x
            float cy; // Center y
            float rx; // Radius x
            float ry; // Radius y
        };

    public:
        path_ellipse(const sgp_point &pt1, const sgp_point &pt2, float alpha_start = 0.0f, float alpha_end = M_PI * 2) : _pt1(pt1),
                                                                                                                         _pt2(pt2),
                                                                                                                         _alpha_start(alpha_start),
                                                                                                                         _alpha_end(alpha_end)
        {
        }
        virtual ~path_ellipse() {}

        /**
         * @brief Draw the ellipse using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            auto points = get_ellipse_points(_pt1, _pt2, _alpha_start, _alpha_end);

            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);
            if (style.width == 1.0f)
            {
                sgp_draw_lines_strip(points.data(), points.size());
            }
            else
            {
                for (int i = 1; i < points.size() - 1; i++)
                {
                    path_line::draw_thik_line(points[i - 1], points[i], style.width);
                }
            }
        }

        /**
         * @brief Fill the ellipse using the fill style
         */
        void fill(const fill_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

            auto triangles = get_ellipse_triangles(_pt1, _pt2, _alpha_start, _alpha_end);

            sgp_draw_filled_triangles(triangles.data(), triangles.size());
        }

        /**
         * @brief Get an array of points approximating the ellipse
         *
         * The number of points is calculated based on the ellipse perimeter to have a good approximation
         *
         * @param start The bounding box start point
         * @param end The bounding box end point
         * @param alpha_start The starting angle in radians
         * @param alpha_end The ending angle in radians
         * @return std::vector<sgp_point> The array of points
         */
        static std::vector<sgp_point> get_ellipse_points(sgp_point start, sgp_point end, float alpha_start = 0.0f, float alpha_end = M_PI * 2)
        {
            auto ed = get_ellipse_data(start, end);

            float alpha = alpha_start;

            float perimeter = 2 * M_PI * std::sqrt((ed.rx * ed.rx + ed.ry * ed.ry) / 2.0f);
            float alpha_step = (2 * M_PI) / perimeter;

            std::vector<sgp_point> points;

            while (alpha <= alpha_end)
            {
                float px = ed.cx + (std::cos(alpha) * ed.rx);
                float py = ed.cy + (std::sin(alpha) * ed.ry);
                points.emplace_back(sgp_point{px, py});

                alpha += alpha_step;
            }

            if (alpha_end - alpha < alpha_step)
            {
                alpha = alpha_end;
                float px = ed.cx + (std::cos(alpha) * ed.rx);
                float py = ed.cy + (std::sin(alpha) * ed.ry);
                points.emplace_back(sgp_point{px, py});
            }

            return points;
        }

        static std::vector<sgp_triangle> get_ellipse_triangles(sgp_point start, sgp_point end, float alpha_start = 0.0f, float alpha_end = M_PI * 2)
        {
            auto points = get_ellipse_points(start, end, alpha_start, alpha_end);
            auto ed = get_ellipse_data(start, end);
            sgp_point center_point{ed.cx, ed.cy};
            std::vector<sgp_triangle> triangles;

            for (int i = 1; i < points.size(); i++)
            {
                sgp_triangle t{center_point, points[i - 1], points[i]};
                triangles.emplace_back(t);
            }

            return triangles;
        }

        static ellipse_data get_ellipse_data(sgp_point start, sgp_point end)
        {
            ellipse_data e;

            e.rx = (end.x - start.x) / 2.0f;
            e.ry = (end.y - start.y) / 2.0f;
            e.cx = start.x + e.rx;
            e.cy = start.y + e.ry;

            return e;
        }

    protected:
        sgp_point _pt1;
        sgp_point _pt2;
        float _alpha_start;
        float _alpha_end;
    };

    /**
     * @brief A rounded rectangle path element
     */
    class path_roundrect : public abstract_sub_path
    {
    public:
        path_roundrect(const sgp_point &pt1, const sgp_point &pt2, float rx, float ry) : _pt1(pt1),
                                                                                         _pt2(pt2),
                                                                                         _rx(rx),
                                                                                         _ry(ry)
        {
        }
        virtual ~path_roundrect() {}

        /**
         * @brief Draw the rounded rectangle using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            auto arc_top_left = path_ellipse::get_ellipse_points(sgp_point{_pt1.x, _pt1.y},
                                                                 sgp_point{_pt1.x + _rx * 2, _pt1.y + _ry * 2},
                                                                 M_PI,
                                                                 M_PI_2 * 3.0f);

            auto arc_top_right = path_ellipse::get_ellipse_points(sgp_point{_pt2.x - _rx * 2, _pt1.y},
                                                                  sgp_point{_pt2.x, _pt1.y + _ry * 2},
                                                                  M_PI_2 * 3.0f,
                                                                  M_PI * 2.0f);

            auto arc_bottom_right = path_ellipse::get_ellipse_points(sgp_point{_pt2.x - _rx * 2, _pt2.y - _ry * 2},
                                                                     sgp_point{_pt2.x, _pt2.y},
                                                                     0,
                                                                     M_PI_2);

            auto arc_bottom_left = path_ellipse::get_ellipse_points(sgp_point{_pt1.x, _pt2.y - _ry * 2},
                                                                    sgp_point{_pt1.x + _rx * 2, _pt2.y},
                                                                    M_PI_2,
                                                                    M_PI);

            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

            if (style.width == 1.0f)
            {
                std::array<sgp_line, 4> lines = {sgp_line{sgp_point{_pt1.x + _rx, _pt1.y}, sgp_point{_pt2.x - _rx, _pt1.y}},
                                                 sgp_line{sgp_point{_pt2.x, _pt1.y + _ry}, sgp_point{_pt2.x, _pt2.y - _ry}},
                                                 sgp_line{sgp_point{_pt2.x - _rx, _pt2.y}, sgp_point{_pt1.x + _rx, _pt2.y}},
                                                 sgp_line{sgp_point{_pt1.x, _pt2.y - _ry}, sgp_point{_pt1.x, _pt1.y + _ry}}};

                sgp_draw_lines(lines.data(), lines.size());

                sgp_draw_lines_strip(arc_top_left.data(), arc_top_left.size());
                sgp_draw_lines_strip(arc_top_right.data(), arc_top_right.size());
                sgp_draw_lines_strip(arc_bottom_right.data(), arc_bottom_right.size());
                sgp_draw_lines_strip(arc_bottom_left.data(), arc_bottom_left.size());
            }
            else
            {
                path_line::draw_thik_line(sgp_point{_pt1.x + _rx, _pt1.y}, sgp_point{_pt2.x - _rx, _pt1.y}, style.width);
                path_line::draw_thik_line(sgp_point{_pt2.x, _pt1.y + _ry}, sgp_point{_pt2.x, _pt2.y - _ry}, style.width);
                path_line::draw_thik_line(sgp_point{_pt2.x - _rx, _pt2.y}, sgp_point{_pt1.x + _rx, _pt2.y}, style.width);
                path_line::draw_thik_line(sgp_point{_pt1.x, _pt2.y - _ry}, sgp_point{_pt1.x, _pt1.y + _ry}, style.width);

                path_line::draw_thik_lines(arc_top_left, style.width);
                path_line::draw_thik_lines(arc_top_right, style.width);
                path_line::draw_thik_lines(arc_bottom_left, style.width);
                path_line::draw_thik_lines(arc_bottom_right, style.width);
            }
        }

        /**
         * @brief Fill the rounded rectangle using the fill style
         */
        void fill(const fill_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

            auto arc_top_left = path_ellipse::get_ellipse_triangles(sgp_point{_pt1.x, _pt1.y},
                                                                    sgp_point{_pt1.x + _rx * 2, _pt1.y + _ry * 2},
                                                                    M_PI,
                                                                    M_PI_2 * 3.0f);

            auto arc_top_right = path_ellipse::get_ellipse_triangles(sgp_point{_pt2.x - _rx * 2, _pt1.y},
                                                                     sgp_point{_pt2.x, _pt1.y + _ry * 2},
                                                                     M_PI_2 * 3.0f,
                                                                     M_PI * 2.0f);

            auto arc_bottom_right = path_ellipse::get_ellipse_triangles(sgp_point{_pt2.x - _rx * 2, _pt2.y - _ry * 2},
                                                                        sgp_point{_pt2.x, _pt2.y},
                                                                        0,
                                                                        M_PI_2);

            auto arc_bottom_left = path_ellipse::get_ellipse_triangles(sgp_point{_pt1.x, _pt2.y - _ry * 2},
                                                                       sgp_point{_pt1.x + _rx * 2, _pt2.y},
                                                                       M_PI_2,
                                                                       M_PI);

            sgp_draw_filled_rect(_pt1.x + _rx, _pt1.y, _pt2.x - _pt1.x - _rx * 2, _pt2.y - _pt1.y);
            sgp_draw_filled_rect(_pt1.x, _pt1.y + _ry, _pt2.x - _pt1.x, _pt2.y - _pt1.y - _ry * 2);

            sgp_draw_filled_triangles(arc_top_left.data(), arc_top_left.size());
            sgp_draw_filled_triangles(arc_top_right.data(), arc_top_right.size());
            sgp_draw_filled_triangles(arc_bottom_right.data(), arc_bottom_right.size());
            sgp_draw_filled_triangles(arc_bottom_left.data(), arc_bottom_left.size());
        }

    protected:
        sgp_point _pt1;
        sgp_point _pt2;
        float _rx;
        float _ry;
    };

    class sub_path : public abstract_sub_path
    {
    public:
        sub_path() {}
        virtual ~sub_path() {}

        void move_to(const sgp_point &pt)
        {
            _points.emplace_back(pt);
        }

        void line_to(const sgp_point &pt)
        {
            if (_points.empty())
                return;

            _points.emplace_back(pt);
        }

        void arc_to(const sgp_point &pt1, const sgp_point &pt2, float radius)
        {
            if (_points.empty())
                return;

            auto arc_points = get_arc_to_points(_points.back(), pt1, pt2, radius);

            _points.insert(_points.end(), arc_points.begin(), arc_points.end());
        }

        void close_path()
        {
            if (_points.empty())
                return;
            _points.emplace_back(_points.front());
        }

        void stroke(const stroke_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

            if (style.width == 1.0f)
            {
                sgp_draw_lines_strip(_points.data(), _points.size());
            }
            else
            {
                for (int i = 1; i < _points.size() - 1; i++)
                {
                    path_line::draw_thik_line(_points[i - 1], _points[i], style.width);
                }
            }
        }

        void fill(const fill_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

            auto triangles = triangulate_polygon(_points);

            sgp_draw_filled_triangles(triangles.data(), triangles.size());
        }

        static float cross_product(const sgp_point &a, const sgp_point &b, const sgp_point &c)
        {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }

        static float distance(const sgp_point &p1, const sgp_point &p2)
        {
            return std::sqrt(std::pow(p2.x - p1.x, 2) + std::pow(p2.y - p1.y, 2));
        }

        static float angle_between_vectors(const sgp_point &p1, const sgp_point &p2, const sgp_point &p3)
        {
            double v1_x = p2.x - p1.x;
            double v1_y = p2.y - p1.y;
            double v2_x = p3.x - p2.x;
            double v2_y = p3.y - p2.y;
            double dot = v1_x * v2_x + v1_y * v2_y;
            double mag1 = std::sqrt(v1_x * v1_x + v1_y * v1_y);
            double mag2 = std::sqrt(v2_x * v2_x + v2_y * v2_y);
            return std::acos(dot / (mag1 * mag2));
        }

        static std::vector<sgp_point> get_arc_to_points(const sgp_point &p0, const sgp_point &p1, const sgp_point &p2, float radius)
        {

            std::vector<sgp_point> arcPoints;

            // Direction vectors
            float dx1 = p0.x - p1.x;
            float dy1 = p0.y - p1.y;
            float dx2 = p2.x - p1.x;
            float dy2 = p2.y - p1.y;

            // Normalize direction vectors
            float len1 = std::hypot(dx1, dy1);
            float len2 = std::hypot(dx2, dy2);
            dx1 /= len1;
            dy1 /= len1;
            dx2 /= len2;
            dy2 /= len2;

            // Compute angle between vectors
            float angle = std::acos(dx1 * dx2 + dy1 * dy2);
            float tanHalfAngle = std::tan(angle / 2.0);

            // Distance from intersection point to tangent points
            float dist = radius / tanHalfAngle;

            // Compute tangent points
            sgp_point tangent1 = {p1.x + dx1 * dist, p1.y + dy1 * dist};
            sgp_point tangent2 = {p1.x + dx2 * dist, p1.y + dy2 * dist};

            // Compute arc center
            float bisectX = dx1 + dx2;
            float bisectY = dy1 + dy2;
            float bisectLen = std::hypot(bisectX, bisectY);
            bisectX /= bisectLen;
            bisectY /= bisectLen;

            sgp_point center = {
                p1.x + bisectX * radius / std::sin(angle / 2.0f),
                p1.y + bisectY * radius / std::sin(angle / 2.0f)};

            // Compute start and end angles
            float startAngle = std::atan2(tangent1.y - center.y, tangent1.x - center.x);
            float endAngle = std::atan2(tangent2.y - center.y, tangent2.x - center.x);

            // Determine arc direction
            bool clockwise = (dx1 * dy2 - dy1 * dx2) < 0;

            // Compute angle difference
            float deltaAngle = endAngle - startAngle;
            if (clockwise && deltaAngle < 0)
                deltaAngle += 2 * M_PI;
            if (!clockwise && deltaAngle > 0)
                deltaAngle -= 2 * M_PI;

            // Estimate arc length
            float arcLength = std::abs(deltaAngle) * radius;

            // Automatically determine number of segments (1 point every ~2 pixels)
            int segments = std::max(4, static_cast<int>(arcLength / 2.0));

            // Generate arc points
            for (int i = 0; i <= segments; ++i)
            {
                float theta = startAngle + deltaAngle * i / segments;
                arcPoints.push_back(sgp_point{center.x + radius * std::cos(theta), center.y + radius * std::sin(theta)});
            }

            return arcPoints;
        }

        static float cross(const sgp_point &a, const sgp_point &b, const sgp_point &c)
        {
            return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
        }

        static bool is_convex(const sgp_point &prev, const sgp_point &curr, const sgp_point &next)
        {
            return cross(prev, curr, next) < 0;
        }

        static bool point_in_triangle(const sgp_point &p, const sgp_triangle &t)
        {
            double d1 = cross(t.a, t.b, p);
            double d2 = cross(t.b, t.c, p);
            double d3 = cross(t.c, t.a, p);
            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);
            return !(has_neg && has_pos);
        }

        static std::vector<sgp_triangle> triangulate_polygon(std::vector<sgp_point> polygon)
        {
            std::vector<sgp_triangle> triangles;
            int n = polygon.size();
            if (n < 3)
                return triangles;

            std::vector<int> indices(n);
            for (int i = 0; i < n; ++i)
                indices[i] = i;

            while (indices.size() > 3)
            {
                bool ear_found = false;
                for (size_t i = 0; i < indices.size(); ++i)
                {
                    int prev = indices[(i + indices.size() - 1) % indices.size()];
                    int curr = indices[i];
                    int next = indices[(i + 1) % indices.size()];

                    sgp_point a = polygon[prev];
                    sgp_point b = polygon[curr];
                    sgp_point c = polygon[next];

                    if (!is_convex(a, b, c))
                        continue;

                    sgp_triangle ear = {a, b, c};
                    bool contains_point = false;
                    for (int j : indices)
                    {
                        if (j == prev || j == curr || j == next)
                            continue;
                        if (point_in_triangle(polygon[j], ear))
                        {
                            contains_point = true;
                            break;
                        }
                    }

                    if (!contains_point)
                    {
                        triangles.push_back(ear);
                        indices.erase(indices.begin() + i);
                        ear_found = true;
                        break;
                    }
                }

                if (!ear_found)
                {
                    return std::vector<sgp_triangle>{}; // Fallback in case of failure
                }
            }

            // Triangolo finale
            if (indices.size() == 3)
            {
                triangles.push_back({polygon[indices[0]], polygon[indices[1]], polygon[indices[2]]});
            }

            return triangles;
        }

    protected:
        std::vector<sgp_point> _points;
    };

    class path
    {
    public:
        void begin()
        {
            _elements.clear();
        }

        void add(std::unique_ptr<abstract_sub_path> &&e)
        {
            _elements.emplace_back(std::move(e));
        }

        void stroke(const stroke_style_s &style)
        {
            for (auto &e : _elements)
                e->stroke(style);
        }

        void fill(const fill_style_s &style)
        {
            for (auto &e : _elements)
                e->fill(style);
        }

        bool empty() const
        {
            return _elements.empty();
        }

        sub_path *current_sub_path()
        {
            if (_elements.empty())
                return nullptr;

            return dynamic_cast<sub_path *>(_elements.back().get());
        }

    protected:
        std::vector<std::unique_ptr<abstract_sub_path>> _elements;
    };

    class canvas
    {
    public:
        canvas(int w, int h)
        {
            sgp_begin(w, h);
            sgp_viewport(0, 0, w, h);
        }

        ~canvas()
        {
            // Begin a render pass.
            sg_pass pass = {};
            pass.swapchain = sglue_swapchain();
            sg_begin_pass(&pass);
            // Dispatch all draw commands to Sokol GFX.
            sgp_flush();
            // Finish a draw command queue, clearing it.
            sgp_end();
            // End render pass.
            sg_end_pass();
            // Commit Sokol render.
            sg_commit();
        }

        void begin_path()
        {
            _path.begin();
        }

        void clear()
        {
            sgp_set_color(fill_style.color.r, fill_style.color.g, fill_style.color.b, fill_style.color.a);
            sgp_clear();
        }

        void line(const sgp_point &pt1, const sgp_point &pt2)
        {
            auto e = std::make_unique<path_line>(pt1, pt2);
            _path.add(std::move(e));
        }

        void rectangle(const sgp_point &pt1, const sgp_point &pt2)
        {
            auto e = std::make_unique<path_rect>(pt1, pt2);
            _path.add(std::move(e));
        }

        void roundrect(const sgp_point &pt1, const sgp_point &pt2, float rx, float ry)
        {
            auto e = std::make_unique<path_roundrect>(pt1, pt2, rx, ry);
            _path.add(std::move(e));
        }

        void ellipse(const sgp_point &pt1, const sgp_point &pt2, float alpha_start = 0.0f, float alpha_end = M_PI * 2)
        {
            auto e = std::make_unique<path_ellipse>(pt1, pt2, alpha_start, alpha_end);
            _path.add(std::move(e));
        }

        void move_to(const sgp_point &pt)
        {
            auto e = std::make_unique<sub_path>();
            e->move_to(pt);

            _path.add(std::move(e));
        }

        void line_to(const sgp_point &pt)
        {
            get_current_sub_path(pt)->line_to(pt);
        }

        void arc_to(const sgp_point &pt1, const sgp_point &pt2, float radius)
        {
            get_current_sub_path(pt1)->arc_to(pt1, pt2, radius);
        }

        void close_path()
        {
            get_current_sub_path(sgp_point{0, 0})->close_path();
        }

        void stroke()
        {
            _path.stroke(stroke_style);
        }

        void fill()
        {
            _path.fill(fill_style);
        }

    public:
        stroke_style_s stroke_style;
        fill_style_s fill_style;

    protected:
        path _path;

    protected:
        sub_path *get_current_sub_path(const sgp_point &default_point)
        {
            auto sp = _path.current_sub_path();

            if (!sp)
            {
                _path.add(std::move(std::make_unique<sub_path>()));
                sp = _path.current_sub_path();
                sp->move_to(default_point);
            }

            return sp;
        }
    };
}
