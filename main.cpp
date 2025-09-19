// This is an example on how to set up and use Sokol GP to draw a filled rectangle.

// Includes Sokol GFX, Sokol GP and Sokol APP, doing all implementations.
#define SOKOL_IMPL
#define SOKOL_GLES3

//#include "sokol_gfx.h"
//#include "sokol_gp.h"
#include "sokol_app.h"
//#include "sokol_glue.h"
#include "sokol_log.h"

#include "io2d.h"

#include <iostream>
#include <cstdlib>
#include <cmath>

// Called on every frame of the application.
static void frame(void) {
    // Get current window size.
    int width = sapp_width(), height = sapp_height();
    float ratio = width/(float)height;

    io2d::canvas c(width, height);

    c.begin_path();
    c.line(io2d::point_2d(10, 10), io2d::point_2d(50, 50));
    c.rectangle(io2d::point_2d(10, 10), io2d::point_2d(50, 50));

    c.ellipse(io2d::point_2d(100, 100), io2d::point_2d(300, 300));

    c.fill_style.color = io2d::rgba_color(0xff0000);
    c.fill();

    c.stroke_style.width = 3.0f;
    c.stroke_style.color = io2d::rgba_color(0xffffff);
    c.stroke();

    c.begin_path();
    c.ellipse(io2d::point_2d(400, 400), io2d::point_2d(500, 500), M_PI, M_PI_2 * 3.0f);
    c.roundrect(io2d::point_2d(100, 400), io2d::point_2d(400, 600), 20.0f, 20.0f);
    
    c.stroke_style.width = 1.0f;
    c.stroke();
/*
    // Begin recording draw commands for a frame buffer of size (width, height).
    sgp_begin(width, height);
    // Set frame buffer drawing region to (0,0,width,height).
    sgp_viewport(0, 0, width, height);
    // Set drawing coordinate space to (left=-ratio, right=ratio, top=1, bottom=-1).
    sgp_project(-ratio, ratio, 1.0f, -1.0f);

    // Clear the frame buffer.
    sgp_set_color(0.1f, 0.1f, 0.1f, 1.0f);
    sgp_clear();

    // Draw an animated rectangle that rotates and changes its colors.
    float time = sapp_frame_count() * sapp_frame_duration();
    float r = std::sin(time)*0.5+0.5, g = std::cos(time)*0.5+0.5;
    sgp_set_color(r, g, 0.3f, 1.0f);
    sgp_rotate_at(time, 0.0f, 0.0f);
    sgp_draw_filled_rect(-0.5f, -0.5f, 1.0f, 1.0f);

    // Begin a render pass.
    sg_pass pass = {.swapchain = sglue_swapchain()};
    sg_begin_pass(&pass);
    // Dispatch all draw commands to Sokol GFX.
    sgp_flush();
    // Finish a draw command queue, clearing it.
    sgp_end();
    // End render pass.
    sg_end_pass();
    // Commit Sokol render.
    sg_commit();
*/    
}

// Called when the application is initializing.
static void init(void) {
    // Initialize Sokol GFX.
    sg_desc sgdesc {};
    sgdesc.environment = sglue_environment();
    sgdesc.logger.func = slog_func;

    sg_setup(&sgdesc);
    if (!sg_isvalid()) {
        std::cerr << "Failed to create Sokol GFX context!" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Initialize Sokol GP, adjust the size of command buffers for your own use.
    sgp_desc sgpdesc = {0};
    sgp_setup(&sgpdesc);
    if (!sgp_is_valid()) {
        std::cerr << "Failed to create Sokol GP context: " << sgp_get_error_message(sgp_get_last_error()) << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

// Called when the application is shutting down.
static void cleanup(void) {
    // Cleanup Sokol GP and Sokol GFX resources.
    sgp_shutdown();
    sg_shutdown();
}

// Implement application main through Sokol APP.
sapp_desc sokol_main(int argc, char* argv[]) {
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
