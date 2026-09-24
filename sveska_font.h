/** @addtogroup lapis
 * @{
 */
/**
 * @file
 */

#ifndef SVESKA_FONT_H
#define SVESKA_FONT_H

#include <gfx/context.h>
#include <gfx/bitmap.h>
#include <io/pixelmap.h>
#include "stb_truetype.h"
struct sveska;
typedef struct sveska sveska_t;

typedef struct {
    pixel_t *data;
    sysarg_t width;
    sysarg_t height;
} font_surface_t;

#define MAX_FONT_NAME_LEN 128

typedef struct sveska_font {
    stbtt_fontinfo info;               // Font info (stb_truetype)
    
    float size;                        // Requested pixel height
    float scale;                       // Scaling factor from font units to pixels
    int ascent;                        // Font ascent
    int descent;                       // Font descent
    int linegap;                       // Line gap
    int baseline;                      // Calculated baseline (usually ascent * scale)
    int line_height;                  // Final line height used for layout

    int bbox_x0, bbox_y0;              // Font bounding box
    int bbox_x1, bbox_y1;

    int char_width;                    // Fixed character width (if needed)
    int space_width;                   // Width of space character

    gfx_rect_t glyphs[128];            // Optional glyph cache / bounding rects
    font_surface_t *surface;           // Surface for rendered glyphs

    unsigned char *font_data;          // Raw TTF data
    size_t ttf_size;                   // Size of TTF data in bytes

    char path[128];                    // Full path to TTF file
    char name[MAX_FONT_NAME_LEN];      // Friendly font name (e.g. "arial.ttf")
    int ref_count;                     // Reference counter for shared font use
    bool is_dynamic; 
} sveska_font_t;




typedef struct {
    int x;
    int y;
    int line_height;
    int y_offset;
    int wrap_width;
} text_pos_t;

sveska_font_t *sveska_font_load(sveska_t *sveska, const char *path, float size_px);


void sveska_font_destroy(sveska_font_t *font);


#endif

/** @}
 */