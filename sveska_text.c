

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */


#include "sveska.h"
#include <ui/ui.h>
#include <ui/window.h>
#include <ui/control.h>
#include <ui/fixed.h>
#include <ui/paint.h>
#include <gfx/render.h>
#include <gfx/color.h>
#include <gfx/color.h>


void highlight_selected_text(sveska_t *sveska) {
    if (!has_selection(sveska)) return;

    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);

    // Set highlight color
    gfx_color_t *highlight_color;
    gfx_color_new_rgb_i16(0xCCCC, 0xEEEE, 0xFFFF, &highlight_color);
    gfx_set_color(sveska->window_gc, highlight_color);

    int current_pos = 0;
    int x = sveska->margin_x;
    int y = sveska->text_area_offset_y;
    int selection_start_x = 0;
    int highlight_y = y;
    int highlight_height = sveska->line_height;  // Default height
    bool in_selection = false;
    int current_line = 0;
    int prev_codepoint = 0;  // FIX: Declare prev_codepoint here

    for (size_t span_idx = 0; span_idx < sveska->document.count; span_idx++) {
        text_span_t *span = &sveska->document.spans[span_idx];
        if (!span->text) continue;

        sveska_font_t *font = span->font ? span->font : sveska->font;
        float scale = span->font_size > 0 ? 
                    stbtt_ScaleForPixelHeight(&font->info, span->font_size) : 
                    font->scale;

        // FIX: Reset kerning for each new span
        prev_codepoint = 0;

        for (size_t i = 0; i < span->length; i++) {
            char c = span->text[i];

            if (current_pos == start) {
                in_selection = true;
                selection_start_x = x;
                
                // Get the correct line for the current position
                if (current_line < (int)sveska->line_count) {
                    highlight_y = sveska->line_y_positions[current_line];
                    highlight_height = sveska->line_heights[current_line];
                }
            }

            if (current_pos == end && in_selection) {
                gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
                    .p0 = {selection_start_x, highlight_y},
                    .p1 = {x, highlight_y + highlight_height}
                });
                in_selection = false;
            }

            if (c == '\n') {
                if (in_selection) {
                    // Highlight to the end of the line
                    gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
                        .p0 = {selection_start_x, highlight_y},
                        .p1 = {sveska->window_width - sveska->margin_x, 
                              highlight_y + highlight_height}
                    });
                    selection_start_x = sveska->margin_x;
                }
                x = sveska->margin_x;
                current_line++;
                prev_codepoint = 0;  // FIX: Reset kerning
                
                // Update highlight position for new line
                if (current_line < (int)sveska->line_count) {
                    highlight_y = sveska->line_y_positions[current_line];
                    highlight_height = sveska->line_heights[current_line];
                } else {
                    // For lines beyond our tracking, estimate position
                    highlight_y += highlight_height;
                }
                
                current_pos++;
                continue;
            }

            int codepoint = (unsigned char)c;
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);

            if (prev_codepoint) {
                x += (int)(stbtt_GetCodepointKernAdvance(&font->info, prev_codepoint, codepoint) * scale);
            }

            x += (int)(advance * scale) + sveska->char_spacing;
            prev_codepoint = codepoint;
            current_pos++;
        }
    }

    // Handle selection ending at EOF or empty line
    if (in_selection) {
        gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
            .p0 = {selection_start_x, highlight_y},
            .p1 = {x, highlight_y + highlight_height}
        });
    }

    gfx_color_delete(highlight_color);
}



/************** ISTORIJA */
// Initialize history
void init_history(sveska_t *sveska) {
    sveska->history_capacity = 100;
    sveska->edit_history = malloc(sveska->history_capacity * sizeof(edit_operation_t));
    sveska->history_size = 0;
    sveska->history_pos = 0;
    sveska->in_undo_redo = false;
    
    // Initialize all operations to zero
    memset(sveska->edit_history, 0, sveska->history_capacity * sizeof(edit_operation_t));
}


// Get Y position of a specific line (0-based)
int get_line_y(sveska_t *sveska, size_t line_num) {
    if (!sveska || line_num >= sveska->new_line_count) return -1;
    return sveska->line_y[line_num];
}

// Get total number of lines
size_t get_line_count(sveska_t *sveska) {
    return sveska->new_line_count;
}


void print_line_y_positions(sveska_t *sveska) {
    if (!sveska || sveska->new_line_count == 0) {
       // printf("No lines tracked!\n");
        return;
    }
    
 //   printf("\n=== Tracked Lines: %ld ===\n", sveska->new_line_count);
    for (int i = 0; i <(int) sveska->new_line_count; i++) {
        printf("Line %2d: Y = %4d\n", i, sveska->line_y_positions[i]);
    }
    printf("=============================\n");
}


/** @}
 */
