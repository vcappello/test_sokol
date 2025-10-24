// This is an example on how to set up and use Sokol GP to draw a filled rectangle.

// Includes Sokol GFX, Sokol GP and Sokol APP, doing all implementations.
#define SOKOL_IMPL
#define SOKOL_GLES3

#include "sokol_app.h"
#include "sokol_log.h"

#include "io2d.h"

#include <iostream>
#include <cstdlib>
#include <cmath>

void test_arc_to(io2d::canvas& c)
{
    sgp_point p0 = {50, 120};
    sgp_point p1 = {100, 120};
    sgp_point p2 = {100, 170};

    c.begin_path();
    c.move_to(p0);
    c.arc_to(p1, p2, 50.0f);
    c.close_path();

    c.stroke_style.width = 3.0f;
    c.stroke_style.color = io2d::rgba_color(0xffd4a373);
    c.stroke();

    c.begin_path();
    c.ellipse(sgp_point{p0.x - 5, p0.y - 5}, sgp_point{p0.x + 5, p0.y + 5});
    c.fill_style.color = io2d::rgba_color(0x80ff0000);
    c.fill();

    c.begin_path();
    c.ellipse(sgp_point{p1.x - 5, p1.y - 5}, sgp_point{p1.x + 5, p1.y + 5});
    c.fill_style.color = io2d::rgba_color(0x800000ff);
    c.fill();

    c.begin_path();
    c.ellipse(sgp_point{p2.x - 5, p2.y - 5}, sgp_point{p2.x + 5, p2.y + 5});
    c.fill_style.color = io2d::rgba_color(0x80ff0000);
    c.fill();
}

// Called on every frame of the application.
static void frame(void)
{
    // Get current window size.
    int width = sapp_width(), height = sapp_height();
    float ratio = width / (float)height;

    io2d::canvas c(width, height);

    c.fill_style.color = io2d::rgba_color(0xfffefae0);
    c.clear();

    c.begin_path();
    c.line(sgp_point{10, 10}, sgp_point{50, 50});
    c.rectangle(sgp_point{10, 10}, sgp_point{50, 50});

    c.ellipse(sgp_point{100, 100}, sgp_point{300, 300});

    c.fill_style.color = io2d::rgba_color(0xffe9edc9);
    c.fill();

    c.stroke_style.width = 3.0f;
    c.stroke_style.color = io2d::rgba_color(0xffccd5ae);
    c.stroke();

    c.begin_path();
    c.ellipse(sgp_point{400, 400}, sgp_point{500, 500}, M_PI, M_PI_2 * 3.0f);
    c.roundrect(sgp_point{100, 400}, sgp_point{400, 600}, 20.0f, 20.0f);

    c.fill_style.color = io2d::rgba_color(0xfffaedcd);
    c.fill();

    c.stroke_style.width = 3.0f;
    c.stroke_style.color = io2d::rgba_color(0xffd4a373);
    c.stroke();

    test_arc_to(c);
}    

// Called when the application is initializing.
static void init(void)
{
    // Initialize Sokol GFX.
    sg_desc sgdesc{};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;

    sg_setup(&sgdesc);
    if (!sg_isvalid())
    {
        std::cerr << "Failed to create Sokol GFX context!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Initialize Sokol GP, adjust the size of command buffers for your own use.
    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid())
    {
        std::cerr << "Failed to create Sokol GP context: " << sgp_get_error_message(sgp_get_last_error()) << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

// Called when the application is shutting down.
static void cleanup(void)
{
    // Cleanup Sokol GP and Sokol GFX resources.
    sgp_shutdown();
    sg_shutdown();
}

// Implement application main through Sokol APP.
sapp_desc sokol_main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    sapp_desc app_desc = {};
    app_desc.init_cb = init;
    app_desc.frame_cb = frame;
    app_desc.cleanup_cb = cleanup;
    app_desc.window_title = "Rectangle (Sokol GP)";
    app_desc.logger.func = slog_func;

    return app_desc;
}
