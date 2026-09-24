

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */


#include "sveska.h"
#include <ui/paint.h>
#include <gfx/render.h>
#include <gfx/color.h>





/****************************************************************** */

 uint64_t get_timestamp(void)
{
    struct timespec ts;
    getrealtime(&ts);
    
    // Combine seconds and nanoseconds into a single nanosecond value
    // Using UINT64_C to ensure 64-bit arithmetic
    return UINT64_C(1000000000) * ts.tv_sec + ts.tv_nsec;
}


/******************************************************************** */


void debug_print_selection(sveska_t *sveska, int start, int end) {
    if (!sveska || !sveska->text_buffer || start < 0 || end < 0) {
        printf("Neispravan parametar selekcije\n");
        return;
    }

    // Normalize start/end
    if (start > end) {
        int tmp = start;
        start = end;
        end = tmp;
    }

    // Clamp to text bounds
    start = MAX(0, start);
    end = MIN(end, (int)sveska->text_length - 1);

    //printf("Selected text [%d-%d]: \"", start, end);
    
    // Print selected characters (replace newlines with visible markers)
    for (int i = start; i <= end; i++) {
        char c = sveska->text_buffer[i];
        if (c == '\n') {
            printf("¶");  // Paragraph symbol for newlines
        } else if (c == '\t') {
            printf("→");   // Right arrow for tabs
        } else if (c < 32 || c > 126) {
            printf("\\x%02X", (unsigned char)c);  // Hex codes for control chars
        } else {
            putchar(c);
        }
    }
    printf("\"\n");
}


/*************************************************************************** */

int get_document_length(sveska_t *sveska) {
    if (!sveska || !sveska->document.spans) return 0;
    
    int length = 0;
    for (size_t i = 0; i < sveska->document.count; i++) {
        length += sveska->document.spans[i].length;
    }
    return length;
}


/************************************************* */

/****************************************************** */

text_span_t *get_current_span(sveska_t *sveska) {
    if (!sveska || sveska->document.count == 0 || 
        sveska->current_span >= sveska->document.count) {
        return NULL;
    }
    return &sveska->document.spans[sveska->current_span];
}

/******************************************************* */

bool create_new_span(sveska_t *sveska) {
    if (!sveska) {
        printf("create_new_span: Neispravna instanca aplikacije\n");
        return false;
    }

    // Ensure document capacity with doubling strategy
    if (sveska->document.count >= sveska->document.capacity) {
        size_t new_capacity = sveska->document.capacity ? 
                             sveska->document.capacity * 2 : 
                             INITIAL_SPAN_CAPACITY;
        
        text_span_t *new_spans = realloc(sveska->document.spans, 
                                       new_capacity * sizeof(text_span_t));
        if (!new_spans) {
            printf("Neuspelo povecanje kapaciteta dokumenta na %zu\n", new_capacity);
            return false;
        }
        sveska->document.spans = new_spans;
        sveska->document.capacity = new_capacity;
    }

    // Initialize new span
    text_span_t *span = &sveska->document.spans[sveska->document.count];
    memset(span, 0, sizeof(text_span_t));

    // Allocate initial text buffer
    span->capacity = 256;
    span->text = malloc(span->capacity);
    if (!span->text) {
        printf("Neuspela alokacija spana bafera texta\n");
        return false;
    }
    span->text[0] = '\0';
    span->length = 0;

    // Set formatting attributes from editor state
    span->bold = sveska->bold;
    span->italic = sveska->italic;
    span->underline = sveska->underline;

    // Set font properties
    if (sveska->font) {
        span->font = sveska_font_ref(sveska->font);
        span->font_size = sveska->font_size;
        str_ncpy(span->font_path, MAX_FONT_PATH_LEN, 
                sveska->font->path, str_size(sveska->font->path));
    }

    // Color conversion to RGB565
    if (sveska->color) {
        uint16_t r, g, b;
        gfx_color_get_rgb_i16(sveska->color, &r, &g, &b);
        span->color = ((r & 0xF800) >> 8) | 
                      ((g & 0xFC00) >> 13) | 
                      ((b & 0xF800) >> 11);
    } else {
        span->color = 0x0000; // Default black color
    }

    // Update document state
    sveska->current_span = sveska->document.count;
    sveska->document.count++;
/*
    printf("Created new span %zu (font: %s, bold:%d, italic:%d, size:%.1f)\n",
           sveska->current_span,
           span->font_path,
           span->bold,
           span->italic,
           span->font_size);
*/
    return true;
}


/*************************************************** */

 bool needs_new_span(sveska_t *sveska, text_span_t *span) {
    return span->bold != sveska->bold || 
           span->italic != sveska->italic || 
           span->underline != sveska->underline;
}

/********************************************************** */
void handle_newline(sveska_t *sveska) {
    if (!sveska) return;

    if (has_selection(sveska)) {
        delete_selected_text(sveska);
    }

    size_t span_idx, pos_in_span;
    get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
    text_span_t *span = &sveska->document.spans[span_idx];

    if (pos_in_span > 0 && pos_in_span < span->length) {
        if (!split_span_at(sveska, span_idx, pos_in_span)) return;
        span_idx++;
        pos_in_span = 0;
        span = &sveska->document.spans[span_idx];
    }

    if (span->length >= span->capacity - 1) {
        size_t new_cap = span->capacity * 2;
        char *new_text = realloc(span->text, new_cap);
        if (!new_text) return;
        span->text = new_text;
        span->capacity = new_cap;
    }

    memmove(&span->text[pos_in_span + 1], &span->text[pos_in_span], span->length - pos_in_span);
    span->text[pos_in_span] = '\n';
    span->length++;
    span->text[span->length] = '\0';

    if (!create_new_span(sveska)) return;

    // Move cursor to the start of the new span
    sveska->cursor_index++;
    sveska->current_span = sveska->document.count - 1;

    // Clear selection and force cursor update
    sveska->selection.start_pos = -1;
    sveska->selection.end_pos = -1;
    sveska->cursor_visible = true;
    
    // Immediately update cursor position
    rebuild_display_text(sveska);
    update_cursor_position(sveska);  // This will set the correct position
    sveska_text_render(sveska);
}


int calculate_current_line_height(sveska_t *sveska) {
    if (!sveska || !sveska->font) {
        return 20; // Absolute minimum fallback
    }

    // Case 1: Completely empty document
    if (sveska->document.count == 0 || 
       (sveska->document.count == 1 && sveska->document.spans[0].length == 0)) {
        return sveska->last_line_height > 0 ? 
               sveska->last_line_height : 
               sveska->font->line_height;
    }

    // Case 2: Empty line - use last known good height
    size_t line_start = 0;
    for (size_t i = 0; i < (size_t)sveska->cursor_index; i++) {
        if (sveska->text_buffer[i] == '\n') {
            line_start = i + 1;
        }
    }
    
    if (line_start >= (size_t)sveska->cursor_index) {
        return sveska->last_line_height;
    }

    // Case 3: Line with content - find tallest character
    int max_height = 0;
    for (size_t i = line_start; i < (size_t)sveska->cursor_index; i++) {
        if (sveska->text_buffer[i] == '\n') break;
        
        size_t span_idx, pos_in_span;
        get_span_and_pos(sveska, i, &span_idx, &pos_in_span);
        text_span_t *span = &sveska->document.spans[span_idx];
        sveska_font_t *font = span->font ? span->font : sveska->font;
        
        int codepoint = (unsigned char)sveska->text_buffer[i];
        int x0, y0, x1, y1;
        float scale = span->font_size > 0.0f ? 
                    stbtt_ScaleForPixelHeight(&font->info, span->font_size) : 
                    font->scale;
        
        stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale, &x0, &y0, &x1, &y1);
        int char_height = (y1 - y0) + 4; // Add 4px padding
        
        if (char_height > max_height) {
            max_height = char_height;
        }
    }

    // Update last known height if we found content
    if (max_height > 0) {
        sveska->last_line_height = max_height;
        return max_height;
    }
    
    // Final fallback
    return sveska->font->line_height;
}

/******************************************************** */

void handle_backspace(sveska_t *sveska) {
    if (!sveska) return;

    // If there's a selection, delete selected text
    if (has_selection(sveska)) {
        delete_selected_text(sveska);
        return;
    }

    // No selection, delete character before cursor
    if (sveska->cursor_index <= 0) return;

    size_t span_idx;
    size_t span_pos;
    get_span_and_pos(sveska, sveska->cursor_index - 1, &span_idx, &span_pos);

    // Ensure valid span and position
    if (span_idx >= sveska->document.count) return;
    text_span_t *span = &sveska->document.spans[span_idx];
    if (!span || span->length == 0 || span_pos >= span->length) return;

    // Record the delete operation
    edit_operation_t op = {
        .type = OP_DELETE,
        .data.text.text = str_ndup(&span->text[span_pos], 1), // Deleted character
        .data.text.position = sveska->cursor_index - 1,
        .data.text.length = 1,
        .affected_span = span_idx,
        .timestamp = get_timestamp()
    };
    record_edit(sveska, op);

    // Shift the text left by one position
    memmove(&span->text[span_pos], &span->text[span_pos + 1], span->length - span_pos - 1);
    span->length--;

    // If the span becomes empty and there are multiple spans, remove it
    if (span->length == 0 && sveska->document.count > 1) {
        free(span->text);
        
        // Shift remaining spans left
        for (size_t i = span_idx; i < sveska->document.count - 1; i++) {
            sveska->document.spans[i] = sveska->document.spans[i + 1];
        }
        sveska->document.count--;

        // Update current span index if needed
        if (sveska->current_span >= span_idx) {
            sveska->current_span = (sveska->current_span == span_idx) ? span_idx - 1 : sveska->current_span;
        }
    }

    // Move cursor back
    sveska->cursor_index--;

     // Try to merge with the previous span if possible
     if (span_idx > 0) {
        text_span_t *prev_span = &sveska->document.spans[span_idx - 1];
        
        // Only merge if fonts and styles match
        if (str_cmp(prev_span->font_path, span->font_path) == 0 &&
            _fabs(prev_span->font_size - span->font_size) < 0.1f &&
            prev_span->bold == span->bold && 
            prev_span->italic == span->italic && 
            prev_span->underline == span->underline) {
            
            // Reallocate previous span to merge the two spans
            size_t new_len = prev_span->length + span->length;
            char *new_text = realloc(prev_span->text, new_len + 1);
            if (new_text) {
                prev_span->text = new_text;
                memcpy(prev_span->text + prev_span->length, span->text, span->length);
                prev_span->length = new_len;
                prev_span->text[new_len] = '\0';
                
                // Remove the now-merged span
                free(span->text);
                for (size_t i = span_idx; i < sveska->document.count - 1; i++) {
                    sveska->document.spans[i] = sveska->document.spans[i + 1];
                }
                sveska->document.count--;

                // Update current span index if needed
                if (sveska->current_span >= span_idx) {
                    sveska->current_span = (sveska->current_span == span_idx) ? span_idx - 1 : sveska->current_span;
                }
            }
        }
    }
    sveska->document_modified = true;
    // Rebuild display text, update cursor position and render text
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}




/***************************************************** */

// sveska_util.c
bool initialize_document(sveska_t *sveska) {
    if (!sveska) {
        printf("initialize_document: Neispravna instanca sveske\n");
        return false;
    }

    // Clean up any existing document
    if (sveska->document.spans) {
        for (size_t i = 0; i < sveska->document.count; i++) {
            if (sveska->document.spans[i].text) {
                free(sveska->document.spans[i].text);
            }
        }
        free(sveska->document.spans);
        sveska->document.spans = NULL;
        sveska->document.count = 0;
    }

    // Free line arrays if allocated
    if (sveska->line_y_positions) {
        free(sveska->line_y_positions);
        sveska->line_y_positions = NULL;
    }
    if (sveska->line_heights) {
        free(sveska->line_heights);
        sveska->line_heights = NULL;
    }

    // Initialize fresh document
    sveska->document.capacity = 4;
    sveska->document.spans = calloc(sveska->document.capacity, sizeof(text_span_t));
    if (!sveska->document.spans) {
        printf("Neuspela alokacija niza spanova\n");
        return false;
    }
    // iniciram cuvanje dokumenta
    sveska->current_file[0] = '\0';
    sveska->document_modified = false;

    // Initialize first text span
    sveska->document.count = 1;
    sveska->current_span = 0;
    text_span_t *span = &sveska->document.spans[0];

    span->capacity = 256;
    span->text = malloc(span->capacity);
    if (!span->text) {
        printf("Neuspela alokacija texta spana\n");
        free(sveska->document.spans);
        sveska->document.spans = NULL;
        sveska->document.count = 0;
        return false;
    }

    span->length = 0;
    span->text[0] = '\0';

    // Apply current text styles and font
    span->bold = sveska->bold;
    span->italic = sveska->italic;
    span->underline = sveska->underline;
    span->font = sveska->font ? sveska_font_ref(sveska->font) : NULL;
    span->font_size = (sveska->font_size > 0) ? sveska->font_size : 24.0f;
    if (sveska->current_font_index >= sveska->font_count && sveska->font_count > 0) {
        sveska->current_font_index = 0;
    }
    span->font_index = sveska->current_font_index;

    // Line position tracking
    sveska->line_cap = 16;
    sveska->line_count = 1;
    sveska->line_y_positions = malloc(sveska->line_cap * sizeof(int));
    sveska->line_heights = malloc(sveska->line_cap * sizeof(int));
    
    if (!sveska->line_y_positions || !sveska->line_heights) {
        printf("Neuspela alokacija nizova linija\n");
        if (sveska->line_y_positions) free(sveska->line_y_positions);
        if (sveska->line_heights) free(sveska->line_heights);
        free(span->text);
        free(sveska->document.spans);
        sveska->document.spans = NULL;
        sveska->document.count = 0;
        return false;
    }

    // Set initial line position and height
    sveska->line_y_positions[0] = sveska->text_area_offset_y;
    sveska->line_heights[0] = sveska->line_height; // Use current line height

    // Initialize cursor position
    sveska->cursor_index = 0;
    sveska->char_cursor.x = sveska->margin_x;
    sveska->char_cursor.y = sveska->text_area_offset_y;
    sveska->char_cursor.height = sveska->line_height;
    sveska->char_cursor.visible = true;

    // Clear any selection
    sveska->selection.start_pos = -1;
    sveska->selection.end_pos = -1;
    sveska->selection.is_selecting = false;

    printf("Dokument succeeded.iniciran\n");
    return true;
}



/******************************************************** */
/* Helper to check if there's an active selection */
bool has_selection(sveska_t *sveska) {
    return sveska && 
           sveska->selection.start_pos >= 0 && 
           sveska->selection.end_pos >= 0 &&
           sveska->selection.start_pos != sveska->selection.end_pos;
}

void rebuild_font_metrics(sveska_t *sveska) {
    if (!sveska || !sveska->font) return;
    
    sveska->font->scale = stbtt_ScaleForPixelHeight(&sveska->font->info, sveska->font_size);
    
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&sveska->font->info, &ascent, &descent, &linegap);
    sveska->line_height = (int)((ascent - descent + linegap) * sveska->font->scale);
}

// Optional helper to strip leading/trailing whitespace
void trim_whitespace(char *str) {
    // Trim leading space
    while (*str == ' ' || *str == '\t') {
        str++;
    }

    // Trim trailing space
    char *end = str + str_length(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }
}


int get_current_line(sveska_t *sveska) {
    if (!sveska || !sveska->text_buffer) return 0;
    
    int line = 0;
    for (int i = 0; i < sveska->cursor_index; i++) {
        if (sveska->text_buffer[i] == '\n') {
            line++;
        }
    }
    return line;
}

int get_line_start(sveska_t *sveska, int line) {
    if (!sveska || !sveska->text_buffer || line < 0) return 0;
    
    int current_line = 0;
    for (int i = 0; sveska->text_buffer[i]; i++) {
        if (current_line == line) return i;
        if (sveska->text_buffer[i] == '\n') {
            current_line++;
        }
    }
    return sveska->cursor_index; // Return current position if line not found
}

int get_line_end(sveska_t *sveska, int line) {
    if (!sveska || !sveska->text_buffer || line < 0) return 0;
    
    int start = get_line_start(sveska, line);
    int end = start;
    while (sveska->text_buffer[end] && sveska->text_buffer[end] != '\n') {
        end++;
    }
    return end;
}

int find_vertical_position(sveska_t *sveska, int target_line, int target_x) {
    if (!sveska || target_line < 0) return 0;

    int current_line = 0;
    int x = sveska->margin_x;
  //  int y = sveska->text_area_offset_y;
    size_t global_pos = 0;
    int closest_pos = 0;
    int closest_dist = INT_MAX;
    int prev_codepoint = 0;

    for (size_t span_idx = 0; span_idx < sveska->document.count; span_idx++) {
        text_span_t *span = &sveska->document.spans[span_idx];
        if (!span->text) continue;

        sveska_font_t *font = span->font ? span->font : sveska->font;
        float scale = span->font_size > 0.0f ? 
                    stbtt_ScaleForPixelHeight(&font->info, span->font_size) : 
                    font->scale;

        for (size_t char_idx = 0; char_idx < span->length; char_idx++) {
            if (current_line == target_line) {
                // Check distance to target x position
                int dist = abs(x - target_x);
                if (dist < closest_dist) {
                    closest_dist = dist;
                    closest_pos = global_pos;
                }
            }

            char c = span->text[char_idx];
            if (c == '\n') {
                if (current_line == target_line) {
                    // Check line end position
                    int dist = abs(x - target_x);
                    if (dist < closest_dist) {
                        closest_pos = global_pos + 1;
                    }
                    return closest_pos;
                }
                
                current_line++;
                x = sveska->margin_x;
                prev_codepoint = 0;
                global_pos++;
                continue;
            }

            // Regular character handling
            int codepoint = (unsigned char)c;
            int advance, lsb;
            stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);

            if (prev_codepoint) {
                x += (int)(stbtt_GetCodepointKernAdvance(&font->info, prev_codepoint, codepoint) * scale);
            }

            x += (int)(advance * scale) + sveska->char_spacing;
            prev_codepoint = codepoint;
            global_pos++;
        }
    }

    return closest_pos;
}

int get_line_count_keyboard(sveska_t *sveska) {
    if (!sveska || !sveska->text_buffer) return 1;
    
    int count = 1;
    for (int i = 0; sveska->text_buffer[i]; i++) {
        if (sveska->text_buffer[i] == '\n') {
            count++;
        }
    }
    return count;
}

void move_cursor_left(sveska_t *sveska) {
    if (sveska->cursor_index > 0) {
        // Clear selection
        sveska->selection.start_pos = -1;
        sveska->selection.end_pos = -1;
        
        sveska->cursor_index--;
        update_cursor_position(sveska);
        sveska_text_render(sveska);
        draw_char_cursor(sveska);
      //  gfx_update(sveska->window_gc);
    }
}

void move_cursor_right(sveska_t *sveska) {
    if (sveska->cursor_index < get_document_length(sveska)) {
        // Clear selection
        sveska->selection.start_pos = -1;
        sveska->selection.end_pos = -1;
        
        sveska->cursor_index++;
        update_cursor_position(sveska);
        sveska_text_render(sveska);
        draw_char_cursor(sveska);
       // gfx_update(sveska->window_gc);
    }
}

void move_cursor_up(sveska_t *sveska) {
    int current_line = get_current_line(sveska);
    if (current_line > 0) {
        // Clear selection
        clear_selection(sveska);
        
        // Find closest position in line above
        int target_pos = find_vertical_position(sveska, current_line - 1, sveska->last_cursor_x);
        sveska->cursor_index = target_pos;
        
        // Force update of both cursors
        update_cursor_position(sveska);
        sveska->cursor_visible = true;
        rebuild_display_text(sveska);  // Add this to ensure text is rebuilt
        sveska_text_render(sveska);
        draw_char_cursor(sveska);
      //  gfx_update(sveska->window_gc);
    }
}

void move_cursor_down(sveska_t *sveska) {
    int current_line = get_current_line(sveska);
    if (current_line < get_line_count_keyboard(sveska) - 1) {
        // Clear selection
        clear_selection(sveska);
        
        // Find closest position in line below
        int target_pos = find_vertical_position(sveska, current_line + 1, sveska->last_cursor_x);
        sveska->cursor_index = target_pos;
        
        // Force update of both cursors
        update_cursor_position(sveska);
        sveska->cursor_visible = true;
        rebuild_display_text(sveska);  // Add this to ensure text is rebuilt
        sveska_text_render(sveska);
        draw_char_cursor(sveska);
      //  gfx_update(sveska->window_gc);
    }
}

// Add this helper function
void clear_selection(sveska_t *sveska) {
    sveska->selection.start_pos = -1;
    sveska->selection.end_pos = -1;
    sveska->selection.is_selecting = false;
}

void move_cursor_home(sveska_t *sveska, bool shift_held) {
    // Clear selection unless shift is held
    if (!shift_held) {
        sveska->selection.start_pos = -1;
        sveska->selection.end_pos = -1;
    }
    
    int line_start = get_line_start(sveska, get_current_line(sveska));
    sveska->cursor_index = line_start;
    update_cursor_position(sveska);
    sveska_text_render(sveska);
    draw_char_cursor(sveska);
   // gfx_update(sveska->window_gc);
}

void move_cursor_end(sveska_t *sveska, bool shift_held) {
    // Clear selection unless shift is held
    if (!shift_held) {
        sveska->selection.start_pos = -1;
        sveska->selection.end_pos = -1;
    }
    
    int line_end = get_line_end(sveska, get_current_line(sveska));
    sveska->cursor_index = line_end;
    update_cursor_position(sveska);
    sveska_text_render(sveska);
    draw_char_cursor(sveska);
   // gfx_update(sveska->window_gc);
}




/** @}
 */
