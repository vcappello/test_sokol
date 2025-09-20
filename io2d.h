#include "sokol_gfx.h"
#include "sokol_gp.h"
#include "sokol_glue.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <cmath>
#include <numbers>

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
            a = (argb >> 24) & 0xff;
            r = (argb >> 16) & 0xff;
            g = (argb >> 8) & 0xff;
            b = argb & 0xff;
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
    class path_element
    {
    public:
        path_element() {}
        virtual ~path_element() {}

        virtual void stroke(const stroke_style_s &style) = 0;
        virtual void fill(const fill_style_s &style) = 0;
    };

    /**
     * @brief A line path element
     */
    class path_line : public path_element
    {
    public:
        path_line(const sgp_point &pt1, const sgp_point &pt2) : _pt1(pt1),
                                                              _pt2(pt2)
        {
        }

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
    class path_rect : public path_element
    {
    public:
        path_rect(const sgp_point &pt1, const sgp_point &pt2) : _pt1(pt1),
                                                              _pt2(pt2)
        {
        }

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

    struct ellipse_data
    {
        float rx;
        float ry;
        float cx;
        float cy;
    };

    /**
     * @brief An ellipse path element
     */
    class path_ellipse : public path_element
    {
    public:
        path_ellipse(const sgp_point &pt1, const sgp_point &pt2, float alpha_start = 0.0f, float alpha_end = M_PI * 2) : _pt1(pt1),
                                                                                                                       _pt2(pt2),
                                                                                                                       _alpha_start(alpha_start),
                                                                                                                       _alpha_end(alpha_end)
        {
        }

        /**
         * @brief Draw the ellipse using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);
            if (style.width == 1.0f)
            {
                auto points = get_ellipse_points(_pt1, _pt2, _alpha_start, _alpha_end);
                sgp_draw_lines_strip(points.data(), points.size());
            }
            else
            {
                auto points = get_ellipse_points(_pt1, _pt2, _alpha_start, _alpha_end);

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

            auto points = get_ellipse_points(_pt1, _pt2, _alpha_start, _alpha_end);
            auto ed = get_ellipse_data(_pt1, _pt2);
            sgp_point center_point{ed.cx, ed.cy};
            std::vector<sgp_triangle> triangles;

            for (int i = 1; i < points.size(); i++)
            {
                sgp_triangle t{center_point, points[i - 1], points[i]};
                triangles.emplace_back(t);
            }

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
    class path_roundrect : public path_element
    {
    public:
        path_roundrect(const sgp_point &pt1, const sgp_point &pt2, float rx, float ry) : _pt1(pt1),
                                                                                       _pt2(pt2),
                                                                                       _rx(rx),
                                                                                       _ry(ry)
        {
        }

        /**
         * @brief Draw the rounded rectangle using the stroke style
         */
        void stroke(const stroke_style_s &style) override
        {
            sgp_set_color(style.color.r, style.color.g, style.color.b, style.color.a);

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
            sgp_draw_filled_rect(_pt1.x, _pt1.y, _pt2.x - _pt1.x, _pt2.y - _pt1.y);
        }

    protected:
        sgp_point _pt1;
        sgp_point _pt2;
        float _rx;
        float _ry;
    };

    class path
    {
    public:
        void begin()
        {
            _elements.clear();
        }

        void add(std::unique_ptr<path_element> &&e)
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

    protected:
        std::vector<std::unique_ptr<path_element>> _elements;
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
    };
}
