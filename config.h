#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* Application */
#define CONFIG_APP_ID "org.gk.polypen"
#define CONFIG_APP_NAME "PolyPen"

/* Initial window and canvas size, in pixels */
#define CONFIG_WINDOW_WIDTH 800
#define CONFIG_WINDOW_HEIGHT 600
#define CONFIG_CANVAS_WIDTH 800
#define CONFIG_CANVAS_HEIGHT 600

/* Editor interaction sizes, in pixels */
#define CONFIG_VERTEX_RADIUS 4
#define CONFIG_CONTROL_POINT_RADIUS 4
#define CONFIG_PICK_TOLERANCE 8

/* Rendering defaults */
#define CONFIG_EDGE_WIDTH 2
#define CONFIG_GRID_SPACING 20
#define CONFIG_GRID_DOT_RADIUS 1.0

/* Canvas and grid colors in ARGB format */
#define CONFIG_CANVAS_BACKGROUND UINT32_C(0xFF111122)
#define CONFIG_GRID_COLOR UINT32_C(0x2A8CA6BF)
#define CONFIG_DEMO_RED UINT32_C(0xFFFF3333)
#define CONFIG_DEMO_GREEN UINT32_C(0xFF33FF33)

#endif