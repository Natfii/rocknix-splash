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
    /* 0: "G"  – roughly a “C” shape plus an interior bar to form a “G” */
    "M 530 364 C 557 364 582 356 600 342 L 587 320 C 573 331 553 337 532 337 C 479 337 444 299 444 250 C 444 201 479 163 532 163 C 553 163 573 169 587 180 L 600 158 C 582 144 557 136 530 136 C 463 136 414 186 414 250 C 414 314 463 364 530 364 Z",
    /* Inner “G” bar */
    "M 560 250 L 532 250 L 532 300 L 492 300 L 492 240 L 560 240 Z",

    /* 1: "A" – a simple triangular “A” with a small cross‐bar */
    "M 306 360 L 396 140 L 486 360 L 456 360 L 441 310 L 361 310 L 346 360 L 306 360 Z",
    /* small inner cross‐bar */
    "M 393 260 L 461 260 L 440 220 L 393 260 Z",

    /* 2: "S" – scaled/translated from a standard “S” curve to fit C‐region */
    "M 440 140 C 533 140 573 180 573 220 C 573 260 533 280 467 280 C 400 280 360 300 360 340 C 360 380 400 360 467 360 C 533 360 573 340 573 300 L 533 300 C 533 320 507 340 467 340 C 427 340 400 320 360 300 C 360 280 427 260 467 260 C 533 260 573 240 573 200 C 573 160 533 140 440 140 Z",

    /* 3: "N" – reused verbatim from RockNIX index 4 */
    "M 821.944 140.106 L 821.944 359.628 L 849.084 359.628 L 849.084 186.006 L 960.038 359.628 L 987.178 359.628 L 987.178 140.106 L 960.038 140.106 L 960.038 313.861 L 849.084 140.106 L 821.944 140.106 Z",

    /* 4: "I" – reused verbatim from RockNIX index 5 */
    "M 1034.814 140.106 L 1034.814 359.627 L 1061.955 359.627 L 1061.955 140.106 L 1034.814 140.106 Z",

    /* 5: "X" – reused verbatim from RockNIX index 6 */
    "M 1116.9 359.628 L 1183.554 264.768 L 1250.208 359.628 L 1284 359.628 L 1200.45 241.219 L 1270.96 140.106 L 1238.369 140.106 L 1183.554 218.469 L 1128.74 140.106 L 1096.141 140.106 L 1166.658 241.219 L 1083.109 359.628 L 1116.9 359.628 Z"
};

/*
 * Color definitions for each path component
 * First 4 paths are light green (brand color)
 * Last 4 paths are dark green (secondary color)
 */
const char *svg_colors[] = {
    "rgb(0,200,0)",  /* Light green for "G" */
    "rgb(0,200,0)",  /* cont. for bar */
    "rgb(0,200,0)",  /* Light green for "A" */
    "rgb(0,200,0)",  /* cont. for bar */
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
