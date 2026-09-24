

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
#include <ui/wdecor.h>
#include <ui/resource.h>
#include <gfx/color.h>
#include <io/pixelmap.h>

/* Rebuilds the display text from all spans */
void rebuild_display_text(sveska_t *sveska) {
    if (!sveska || !sveska->document.spans || sveska->document.count == 0) {
        printf("Nema spanova za rekonfiguraciju.\n");
        sveska->text_length = 0;
        if (sveska->text_buffer && sveska->text_capacity > 0) {
            sveska->text_buffer[0] = '\0';  // clear buffer safely
        }
        return;
    }

    // Step 1: Rebuild text buffer
    size_t total_bytes = 0;
    for (size_t i = 0; i < sveska->document.count; i++) {
        if (sveska->document.spans[i].text) {
            total_bytes += sveska->document.spans[i].length;
        }
    }

    if (total_bytes >= sveska->text_capacity) {
        size_t new_capacity = MAX(total_bytes + 1, sveska->text_capacity * 2);
        char *new_buffer = realloc(sveska->text_buffer, new_capacity);
        if (!new_buffer) {
            printf("GRESKA: Neuspelo realociranje texta bafera\n");
            return;
        }
        sveska->text_buffer = new_buffer;
        sveska->text_capacity = new_capacity;
    }

    size_t pos = 0;
    for (size_t i = 0; i < sveska->document.count; i++) {
        if (sveska->document.spans[i].text && sveska->document.spans[i].length > 0) {
            size_t copy_len = MIN(sveska->document.spans[i].length, sveska->text_capacity - pos - 1);
            memcpy(sveska->text_buffer + pos, sveska->document.spans[i].text, copy_len);
            pos += copy_len;
        }
    }
    sveska->text_buffer[pos] = '\0';
    sveska->text_length = pos;
    
    // Step 2: Recalculate line heights
    if (sveska->line_y_positions) free(sveska->line_y_positions);
    if (sveska->line_heights) free(sveska->line_heights);
    
    // Allocate line tracking arrays
    sveska->line_cap = 128;  // Start with capacity for 128 lines
    sveska->line_y_positions = malloc(sveska->line_cap * sizeof(int));
    sveska->line_heights = malloc(sveska->line_cap * sizeof(int));
    sveska->line_count = 0;
    
    if (!sveska->line_y_positions || !sveska->line_heights) {
        printf("GRESKA: Neuspela alokacija nizova linija\n");
        if (sveska->line_y_positions) free(sveska->line_y_positions);
        if (sveska->line_heights) free(sveska->line_heights);
        sveska->line_y_positions = NULL;
        sveska->line_heights = NULL;
        return;
    }
    
    // Initialize first line
    int current_line = 0;
    sveska->line_y_positions[current_line] = sveska->text_area_offset_y;
    sveska->line_heights[current_line] = sveska->line_height;
    sveska->line_count = 1;
    
    int max_line_height = sveska->line_height;
    int x = sveska->margin_x;
    
    // Scan through all spans to calculate line heights
    for (size_t span_idx = 0; span_idx < sveska->document.count; span_idx++) {
        text_span_t *span = &sveska->document.spans[span_idx];
        
        if (span->is_image) {
            // Check if image needs to wrap to new line
            if (x + span->img_width > (sveska->window_width - sveska->margin_x) && 
                x > sveska->margin_x) {
                // Store max height for current line
                sveska->line_heights[current_line] = max_line_height;
                
                // Move to next line
                current_line++;
                if (current_line >= (int)sveska->line_cap) {
                    // Expand capacity
                    sveska->line_cap *= 2;
                    sveska->line_y_positions = realloc(sveska->line_y_positions,
                                                     sveska->line_cap * sizeof(int));
                    sveska->line_heights = realloc(sveska->line_heights,
                                                 sveska->line_cap * sizeof(int));
                    if (!sveska->line_y_positions || !sveska->line_heights) {
                        printf("GRESKA: Neuspela realokacija nizova linija\n");
                        sveska->line_count = current_line;
                        return;
                    }
                }
       // Initialize new line
       sveska->line_y_positions[current_line] = 
       sveska->line_y_positions[current_line - 1] + max_line_height + 5;
   max_line_height = sveska->line_height;  // Reset to default
   sveska->line_heights[current_line] = max_line_height;
   sveska->line_count = current_line + 1;
   x = sveska->margin_x;
}

// Update max height for this line
if (span->img_height > max_line_height) {
   max_line_height = span->img_height;
   sveska->line_heights[current_line] = max_line_height;
}

// Advance position
x += span->img_width + sveska->char_spacing;
continue;
}
        
        if (!span->text) continue;
        
        sveska_font_t *font = span->font ? span->font : sveska->font;
        if (!font) continue;
        
        float scale = span->font_size > 0 ? 
                    stbtt_ScaleForPixelHeight(&font->info, span->font_size) : 
                    font->scale;
        
        for (size_t i = 0; i < span->length; i++) {
            char c = span->text[i];
            
            if (c == '\n') {
                // Store max height for current line
                sveska->line_heights[current_line] = max_line_height;
                
                // Move to next line
                current_line++;
                if (current_line >= (int)sveska->line_cap) {
                    // Expand capacity
                    sveska->line_cap *= 2;
                    sveska->line_y_positions = realloc(sveska->line_y_positions,
                                                     sveska->line_cap * sizeof(int));
                    sveska->line_heights = realloc(sveska->line_heights,
                                                 sveska->line_cap * sizeof(int));
                    if (!sveska->line_y_positions || !sveska->line_heights) {
                        printf("GRESKA: Neuspela realokacija nizova linija\n");
                        sveska->line_count = current_line;
                        return;
                    }
                }
                
                // Initialize new line
                sveska->line_y_positions[current_line] = 
                    sveska->line_y_positions[current_line - 1] + max_line_height + 5; // Add 5px spacing
                max_line_height = sveska->line_height;  // Reset to default
                sveska->line_heights[current_line] = max_line_height;
                sveska->line_count = current_line + 1;
                x = sveska->margin_x;
                continue;
            }
            
            // Regular character
            int codepoint = (unsigned char)c;
            int advance, lsb, x0, y0, x1, y1;
            stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);
            stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
            
            // Update max height for this line
            int char_height = (y1 - y0) + 4; // Add 4px padding
            if (char_height > max_line_height) {
                max_line_height = char_height;
                sveska->line_heights[current_line] = max_line_height;
            }
            
            // Advance position
            x += (int)(advance * scale) + sveska->char_spacing;
        }
    }
    
    // Store max height for last line
    sveska->line_heights[current_line] = max_line_height;
}






/******************************************************************** */
void sveska_text_render(sveska_t *sveska) {
    if (!sveska || !sveska->window_gc || !sveska->document.spans) {
        return;
    }

    // Clear background
    gfx_set_color(sveska->window_gc, sveska->bkg_color);
    draw_background();

    // Highlight any selected text first
    highlight_selected_text(sveska);

    // Reset line tracking
    sveska->line_count = 0;
    int initial_y = sveska->text_area_offset_y;

    // Ensure capacity for initial line
    if (sveska->line_cap < 1) {
        sveska->line_cap = 16;
        sveska->line_y_positions = realloc(sveska->line_y_positions, 
                                         sveska->line_cap * sizeof(int));
        sveska->line_heights = realloc(sveska->line_heights,
                                     sveska->line_cap * sizeof(int));
    }

    // Store initial line position and height
    sveska->line_y_positions[0] = initial_y;
    sveska->line_heights[0] = sveska->line_height;  // Default height
    sveska->line_count = 1;

    // Initial rendering position
    int x = sveska->margin_x;
    int y = initial_y;
    int current_line = 0;

    // Get main font metrics for reference
    int main_ascent, main_descent, main_linegap;
    stbtt_GetFontVMetrics(&sveska->font->info, &main_ascent, &main_descent, &main_linegap);
    float main_scale = stbtt_ScaleForPixelHeight(&sveska->font->info, sveska->font_size);
    int main_baseline_y = y + (int)(main_ascent * main_scale);

    // Get color components for blending
    uint16_t bg_r, bg_g, bg_b;
    gfx_color_get_rgb_i16(sveska->bkg_color, &bg_r, &bg_g, &bg_b);
    uint16_t fg_r, fg_g, fg_b;
    gfx_color_get_rgb_i16(sveska->color, &fg_r, &fg_g, &fg_b);

    // Track the maximum line height for this line
    int current_line_max_height = sveska->line_height;

    size_t global_pos = 0;
    
    size_t char_global_pos = 0;
    // Render each text span
    for (size_t span_idx = 0; span_idx < sveska->document.count; span_idx++) {
        text_span_t *span = &sveska->document.spans[span_idx];
// podrska za sliku
if (span->is_image) {
    // Check if image needs to wrap to new line
    if (x + span->img_width > (sveska->window_width - sveska->margin_x) && 
        x > sveska->margin_x) {
        // Update line tracking
        sveska->line_heights[current_line] = current_line_max_height;
        current_line++;
        if (sveska->line_count >= sveska->line_cap) {
            sveska->line_cap *= 2;
            sveska->line_y_positions = realloc(sveska->line_y_positions,
                                             sveska->line_cap * sizeof(int));
            sveska->line_heights = realloc(sveska->line_heights,
                                         sveska->line_cap * sizeof(int));
        }
        
        // Initialize new line
        x = sveska->margin_x;
        y += current_line_max_height + 5;
        sveska->line_y_positions[sveska->line_count] = y;
        sveska->line_heights[sveska->line_count] = sveska->line_height;
        sveska->line_count++;
        
        current_line_max_height = sveska->line_height;
        main_baseline_y = y + (int)(main_ascent * main_scale);
    }

    // Update current line max height
    if (span->img_height > current_line_max_height) {
        current_line_max_height = span->img_height;
        sveska->line_heights[current_line] = current_line_max_height;
    }

    // Draw image placeholder
    gfx_set_color(sveska->window_gc, sveska->color);
    gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
        .p0 = {x, y},
        .p1 = {x + span->img_width, y + span->img_height}
    });

    // Advance position
    x += span->img_width + sveska->char_spacing;
    continue;
}
// kraj podrske za sliku
     
        if (!span->text || span->length == 0) continue;
        
        sveska_font_t *font = span->font ? span->font : sveska->font;
        if (!font) continue;
        
        // Get font metrics for current span
        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &linegap);
        float scale = stbtt_ScaleForPixelHeight(&font->info, 
                      span->font_size > 0 ? span->font_size : sveska->font_size);

        // Calculate vertical shift
        int span_baseline_y = y + (int)(ascent * scale);
        int vertical_shift = main_baseline_y - span_baseline_y;
        
        // Update current line's maximum height
        int span_line_height = (int)((ascent - descent + linegap) * scale);
        if (span_line_height > current_line_max_height) {
            current_line_max_height = span_line_height;
            sveska->line_heights[current_line] = current_line_max_height;
        }
        
        // Set up rendering effects
        int bold_offset = span->bold ? 1 : 0;
        float italic_skew = span->italic ? 0.25f : 0.0f;
        
        // Track kerning
        int prev_codepoint = 0;
       
        // Render each character
        for (size_t i = 0; i < span->length; ) {

                        // Track character's global position
      
             char_global_pos = global_pos + i;

            // Get UTF-8 character
            utf8_char_t ch = sveska->get_utf8_char(span->text, i);
            if (ch.length == 0) {
                i++;
                global_pos++;
                continue;
            }
            
            // Get Unicode codepoint
            int codepoint = 0;
            if (ch.length == 1) {
                codepoint = ch.bytes[0];
            } else if (ch.length == 2) {
                codepoint = ((ch.bytes[0] & 0x1F) << 6) | (ch.bytes[1] & 0x3F);
            } else if (ch.length == 3) {
                codepoint = ((ch.bytes[0] & 0x0F) << 12) | ((ch.bytes[1] & 0x3F) << 6) | (ch.bytes[2] & 0x3F);
            } else if (ch.length == 4) {
                codepoint = ((ch.bytes[0] & 0x07) << 18) | ((ch.bytes[1] & 0x3F) << 12) | 
                           ((ch.bytes[2] & 0x3F) << 6) | (ch.bytes[3] & 0x3F);
            }
            
            // Handle newlines
            if (codepoint == '\n') {

                sveska->line_heights[current_line] = current_line_max_height;
                x = sveska->margin_x;
                y += current_line_max_height + (current_line_max_height / 5);
                current_line++;
                current_line_max_height = sveska->line_height;  // Reset for new line
                
                // Update line tracking
        // Update line tracking
        if (sveska->line_count >= sveska->line_cap) {
            sveska->line_cap *= 2;
            sveska->line_y_positions = realloc(sveska->line_y_positions,
                                             sveska->line_cap * sizeof(int));
            sveska->line_heights = realloc(sveska->line_heights,
                                         sveska->line_cap * sizeof(int));
        }
        sveska->line_y_positions[sveska->line_count] = y;
        sveska->line_heights[sveska->line_count] = current_line_max_height;
        sveska->line_count++;
                
                // Update font metrics for new line
                main_baseline_y = y + (int)(main_ascent * main_scale);
                prev_codepoint = 0;
                i += ch.length;
                continue;
            }
            
            // Skip carriage returns
            if (codepoint == '\r') {
                i += ch.length;
                continue;
            }
            
            // Get character metrics
            int advance, lsb, x0, y0, x1, y1;
            stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);
            stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
            
            // Apply kerning
            if (prev_codepoint) {
                x += (int)(stbtt_GetCodepointKernAdvance(&font->info, prev_codepoint, codepoint) * scale);
            }
            
            // Word wrap
            int char_width = (x1 - x0) + bold_offset + sveska->char_spacing;
            if (codepoint == ' ') {
                char_width = font->space_width;
            }
            
            if (x + char_width > (sveska->window_width - sveska->margin_x) && x > sveska->margin_x) {
             
                sveska->last_cursor_x = sveska->margin_x;
                
    // FIX: Reset cursor position when wrapping
    if (sveska->cursor_index == (int)char_global_pos) {
        sveska->char_cursor.x = sveska->margin_x;
       
        sveska->char_cursor.height = current_line_max_height;
    }

                x = sveska->margin_x;
                y += calculate_current_line_height(sveska);
                current_line++;
                current_line_max_height = calculate_current_line_height(sveska);  // Reset for new line
                
                // Update line tracking
                if (sveska->line_count >= sveska->line_cap) {
                    sveska->line_cap *= 2;
                    sveska->line_y_positions = realloc(sveska->line_y_positions,
                                                     sveska->line_cap * sizeof(int));
                    sveska->line_heights = realloc(sveska->line_heights,
                                                 sveska->line_cap * sizeof(int));
                    if (!sveska->line_y_positions || !sveska->line_heights) {
                        i += ch.length;
                        continue;
                    }
                }
                sveska->line_y_positions[sveska->line_count] = y;
                sveska->line_heights[sveska->line_count] = current_line_max_height;
                sveska->line_count++;
                
                main_baseline_y = y + (int)(main_ascent * main_scale);
                prev_codepoint = 0;

            }
            
            // Get character bitmap
            int w, h;
            unsigned char *bitmap = stbtt_GetCodepointBitmap(
                &font->info, scale, scale, codepoint, &w, &h, NULL, NULL);
            
            if (bitmap) {
                // Calculate drawing position with vertical shift
                int draw_x = x + x0;
                int draw_y = y + (int)(ascent * scale) + y0 + vertical_shift;
                
                // Draw character pixels
                for (int dy = 0; dy < h; dy++) {
                    int skew_offset = (int)((h - dy - 1) * italic_skew);
                    
                    for (int dx = 0; dx < w; dx++) {
                        uint8_t alpha = bitmap[dy * w + dx];
                        if (alpha > 0) {
                            uint16_t r = (alpha * fg_r + (255 - alpha) * bg_r) / 255;
                            uint16_t g = (alpha * fg_g + (255 - alpha) * bg_g) / 255;
                            uint16_t b = (alpha * fg_b + (255 - alpha) * bg_b) / 255;
                            
                            gfx_color_t *blended;
                            if (gfx_color_new_rgb_i16(r, g, b, &blended) == EOK) {
                                gfx_set_color(sveska->window_gc, blended);
                                
                                // Main pixel
                                gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
                                    .p0 = {draw_x + dx + skew_offset, draw_y + dy},
                                    .p1 = {draw_x + dx + skew_offset + 1, draw_y + dy + 1}
                                });
                                
                                // Bold effect
                                if (bold_offset) {
                                    gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
                                        .p0 = {draw_x + dx + skew_offset + 1, draw_y + dy},
                                        .p1 = {draw_x + dx + skew_offset + 2, draw_y + dy + 1}
                                    });
                                }
                                
                                gfx_color_delete(blended);
                            }
                        }
                    }
                }
                
                stbtt_FreeBitmap(bitmap, NULL);
            }
            


            // Advance position
            global_pos += span->length;
            x += (int)(advance * scale) + sveska->char_spacing;
            prev_codepoint = codepoint;
            i += ch.length;
        }
    }
    if (sveska->line_count > 0) {
        sveska->line_heights[sveska->line_count - 1] = current_line_max_height;
    }
    update_cursor_position(sveska);
    // Final cursor rendering if no selection
    if (!sveska->selection.is_selecting) {
        draw_char_cursor(sveska);
    }
    gfx_update(sveska->window_gc);
}


/****************************************************** */

void draw_background(void) {
    // Get exact drawable area
    gfx_rect_t rect;
    ui_window_get_app_rect(sveska->window, &rect);
    
    // Background area - matches window exactly
    int bg_left = rect.p0.x + 2;          // 2px from left
    int bg_right = rect.p1.x - 2;         // 2px from right 
    int bg_top = 50;                      // Below menu bar
    int bg_bottom = rect.p1.y - 8;        // 8px from bottom

    // Fill background
    gfx_set_color(sveska->window_gc, sveska->bkg_color);
    for (int y = bg_top; y < bg_bottom; y++) {
        for (int x = bg_left; x < bg_right; x++) {
            gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
                .p0 = {x, y}, .p1 = {x+1, y+1}
            });
        }
    }

   // DEBUG: Draw corner markers (remove after testing) 
    gfx_set_color(sveska->window_gc, sveska->color);
    
    // Top-left corner marker (5x5 px)
    gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
        .p0 = {bg_left, bg_top}, 
        .p1 = {bg_left+5, bg_top+5}
    });
    
    // Bottom-right corner marker (5x5 px)
    gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
        .p0 = {bg_right-5, bg_bottom-5},
        .p1 = {bg_right, bg_bottom}
    });

   // gfx_update(sveska->window_gc);
}

/********************************************************************** */

void draw_char_cursor(sveska_t *sveska) {
    if (!sveska || !sveska->window_gc) return;
    
    // Draw cursor only if not selecting
    if (!sveska->selection.is_selecting) {
        gfx_set_color(sveska->window_gc, sveska->highlight_color);
        gfx_fill_rect(sveska->window_gc, &(gfx_rect_t){
            .p0 = {sveska->char_cursor.x, sveska->char_cursor.y},
            .p1 = {sveska->char_cursor.x + 2, sveska->char_cursor.y + sveska->char_cursor.height}
        });
    }
}



int compute_underline_y(sveska_font_t *font, float scale, int baseline_y) {
    int x0, y0, x1, y1;
    stbtt_GetCodepointBox(&font->info, 'x', &x0, &y0, &x1, &y1);
    return baseline_y + (int)(y1 * scale) + 2;
}




/** @}
 */
