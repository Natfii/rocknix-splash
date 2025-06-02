#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "fbsplash.h"
#include "svg_parser.h"
#include "svg_renderer.h"
#include "dt_rotation.h"

const char *svg_paths[] = {
    /* "G" */
    "M 0 140 L 0 360 L 150 360 L 150 300 L 60 300 L 60 250 L 140 250 L 140 200 L 60 200 L 60 140 L 0 140 Z",
    /* "A" */
    "M 170 360 L 220 140 L 270 360 L 240 360 L 230 310 L 190 310 L 180 360 L 170 360 Z "
    "M 200 260 L 210 220 L 220 260 L 200 260 Z",
    /* "S" */
    "M 330 140 C 400 140 430 180 430 220 C 430 260 400 280 350 280 C 300 280 270 300 270 340 C 270 380 300 360 350 360 C 400 360 430 340 430 300 L 400 300 C 400 320 380 340 350 340 C 320 340 300 320 300 300 C 300 280 320 260 350 260 C 400 260 430 240 430 200 C 430 160 400 140 330 140 Z",
    /* "N" (reused from original ROCKNIX) */
    "M 821.944 140.106 L 821.944 359.628 L 849.084 359.628 L 849.084 186.006 L 960.038 359.628 L 987.178 359.628 L 987.178 140.106 L 960.038 140.106 L 960.038 313.861 L 849.084 140.106 L 821.944 140.106 Z",
    /* "I" (reused from original ROCKNIX) */
    "M 1034.814 140.106 L 1034.814 359.627 L 1061.955 359.627 L 1061.955 140.106 L 1034.814 140.106 Z",
    /* "X" (reused from original ROCKNIX) */
    "M 1116.9 359.628 L 1183.554 264.768 L 1250.208 359.628 L 1284 359.628 L 1200.45 241.219 L 1270.96 140.106 L 1238.369 140.106 L 1183.554 218.469 L 1128.74 140.106 L 1096.141 140.106 L 1166.658 241.219 L 1083.109 359.628 L 1116.9 359.628 Z"
};

/*
 * Color definitions for each path component
 * First 3 paths are light green (brand color)
 * Last 3 paths are dark green (secondary color)
 */
const char *svg_colors[] = {
    "rgb(0,200,0)",  /* Light green for "G" */
    "rgb(0,200,0)",  /* Light green for "A" */
    "rgb(0,200,0)",  /* Light green for "S" */
    "rgb(0,100,0)",  /* Dark green for "N" */
    "rgb(0,100,0)",  /* Dark green for "I" */
    "rgb(0,100,0)"   /* Dark green for "X" */
};

#define NUM_PATHS (sizeof(svg_paths) / sizeof(svg_paths[0]))

/*
 * Main program entry point
 */
int main(void) {
    const char *fb_device = "/dev/fb0";

    // Get rotation from device tree
    int rotation = get_display_rotation();

    // Check framebuffer device accessibility
    if (access(fb_device, R_OK | W_OK) != 0) {
        fprintf(stderr, "Cannot access %s: %s\n", fb_device, strerror(errno));
        return 1;
    }

    // Initialize framebuffer
    Framebuffer *fb = fb_init(fb_device);
    if (!fb) {
        fprintf(stderr, "Failed to initialize framebuffer\n");
        return 1;
    }

    // Calculate display parameters
    DisplayInfo *display_info = calculate_display_info(fb);
    if (!display_info) {
        fprintf(stderr, "Failed to calculate display information\n");
        fb_cleanup(fb);
        return 1;
    }

    // Clear screen to black
    for (uint32_t y = 0; y < fb->vinfo.yres; y++) {
        for (uint32_t x = 0; x < fb->vinfo.xres; x++) {
            set_pixel(fb, x, y, 0x00000000);
        }
    }

    // Process and render each path component
    for (size_t i = 0; i < NUM_PATHS; i++) {
        SVGPath *svg = parse_svg_path(svg_paths[i], svg_colors[i]);
        if (!svg) {
            fprintf(stderr, "Failed to parse SVG path %zu\n", i);
            continue;
        }

        // Apply rotation from device tree if specified
        if (rotation)
            rotate_svg_path(svg, rotation);

        // Render the path
        render_svg_path(fb, svg, display_info);
        free_svg_path(svg);
    }

    // Flush changes to the framebuffer
    fb_flush(fb);

    // Clean up
    free(display_info);
    fb_cleanup(fb);

    return 0;
}
