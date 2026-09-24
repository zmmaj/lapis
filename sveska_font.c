

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>
#include <math.h>
#include "stb_truetype.h"
#include "sveska_font.h"
#include "sveska.h"

#include <gfx/render.h>

extern sveska_t *sveska;


sveska_font_t *sveska_font_load(sveska_t *sveska, const char *path, float target_pixel_height) {
    if (!sveska || !path) return NULL;

    // 1. Check cache for existing font by path and size (allowing slight float diff)
    for (size_t i = 0; i < sveska->font_cache_count; i++) {
        font_cache_entry_t *entry = &sveska->font_cache[i];
        if (str_cmp(entry->path, path) == 0 &&
            fabs(entry->size - target_pixel_height) < 0.1f) {
            entry->font->ref_count++;
            return entry->font;
        }
    }

    // 2. Load font file into memory
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    unsigned char *font_data = malloc(fsize);
    if (!font_data) {
        fclose(f);
        return NULL;
    }
    if (fread(font_data, 1, fsize, f) != (size_t)fsize) {
        free(font_data);
        fclose(f);
        return NULL;
    }
    fclose(f);

    // 3. Initialize font structure
    sveska_font_t *font = calloc(1, sizeof(sveska_font_t));
    if (!font) {
        free(font_data);
        return NULL;
    }

    str_ncpy(font->path, MAX_FONT_PATH_LEN, path, str_size(path));
    font->size = target_pixel_height;
    font->ref_count = 1;
    font->font_data = font_data;

    if (!stbtt_InitFont(&font->info, font_data, 0)) {
        free(font_data);
        free(font);
        return NULL;
    }

    // 4. Get font metrics & scaling
    int ascent, descent, linegap;
    stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &linegap);
    font->scale = stbtt_ScaleForPixelHeight(&font->info, target_pixel_height);

    int x0, y0, x1, y1;
    stbtt_GetCodepointBitmapBox(&font->info, 'X', font->scale, font->scale, &x0, &y0, &x1, &y1);
    int char_height = (y1 - y0) + 4; // Add 4px padding
    font->char_width = (x1 - x0) + 2;  // Add some padding
    font->line_height = char_height;

    int space_advance, space_lsb;
    stbtt_GetCodepointHMetrics(&font->info, ' ', &space_advance, &space_lsb);
    font->space_width = (int)(space_advance * font->scale);
    if (font->space_width == 0) {
        font->space_width = (int)(font->scale * 5); // fallback
    }

    if (sveska) {
        sveska->last_line_height = char_height;
        printf("Initial line height set to %d pixels (from 'X' character)\n", char_height);
    }

    stbtt_GetFontBoundingBox(&font->info,
                            &font->bbox_x0, &font->bbox_y0,
                            &font->bbox_x1, &font->bbox_y1);
/*
    printf("Loaded font '%s' (size %.1f):\n", path, target_pixel_height);
    printf("  Bounding Box: (%d,%d) to (%d,%d)\n",
           font->bbox_x0, font->bbox_y0, font->bbox_x1, font->bbox_y1);
    printf("  Line height: %d pixels\n", font->line_height);
*/
    // 5. Add font to cache if there is space
    if (sveska->font_cache_count < MAX_FONT_CACHE) {
        font_cache_entry_t *entry = &sveska->font_cache[sveska->font_cache_count++];
        str_ncpy(entry->path, MAX_FONT_PATH_LEN, path, str_size(path));
        entry->size = target_pixel_height;
        entry->font = font;
    } else {
        printf("Warning: Font cache full, cannot cache font '%s'\n", path);
        // Not critical — still return loaded font
    }

    // 6. Check for missing Serbian Latin chars (optional)
    int missing_chars = 0;
    const int test_chars[] = {0xC5, 0xC4, 0xE5, 0xE4}; // ŠĐČĆšđčć
    for (int i = 0; i < 4; i++) {
        if (!stbtt_FindGlyphIndex(&font->info, test_chars[i])) {
            missing_chars++;
        }
    }
    if (missing_chars > 0) {
        printf("Warning: Font '%s' is missing %d Serbian Latin characters\n", path, missing_chars);
    }

    return font;
}



    /***************************************************************** */

    // sveska_font.c
void sveska_set_font(sveska_t *sveska, const char *font_path, float size) {
    if (!sveska || !font_path) {
        printf("Invalid parameters to sveska_set_font\n");
        return;
    }

    // Find the font in our pre-loaded list
    size_t font_index = (size_t)-1;
    for (size_t i = 0; i < sveska->font_count; i++) {
        if (str_cmp(sveska->fonts[i].path, font_path) == 0) {
            font_index = i;
            break;
        }
    }

    if (font_index == (size_t)-1) {
        printf("Font not found in pre-loaded list: %s\n", font_path);
        return;
    }

    sveska_font_t *new_font = &sveska->fonts[font_index];
    sveska_font_t *old_font = sveska->font;

    // Update font metrics with new size if needed
    if (size > 0 && size != new_font->size) {
        new_font->size = size;
        new_font->scale = stbtt_ScaleForPixelHeight(&new_font->info, size);
        
        // Recalculate metrics
        new_font->baseline = (int)(new_font->ascent * new_font->scale);
        new_font->line_height = (int)((new_font->ascent - new_font->descent + new_font->linegap) * new_font->scale);
        
        int space_advance, space_lsb;
        stbtt_GetCodepointHMetrics(&new_font->info, ' ', &space_advance, &space_lsb);
        new_font->space_width = (int)(space_advance * new_font->scale);
    }

    // Update current font reference
    sveska->font = new_font;
    sveska->current_font_index = font_index;
    sveska->font_size = new_font->size;
    
    // Update current font path
    str_ncpy(sveska->current_font_path, MAX_FONT_PATH_LEN, 
            new_font->path, str_size(new_font->path));

   // printf("Set font to: %s (%.1f)\n", new_font->path, new_font->size);

    // If we have a document, ensure next text uses this font
    if (sveska->document.count > 0) {
        text_span_t *last_span = &sveska->document.spans[sveska->document.count-1];
        if (last_span->length == 0) {
            // Update empty span's font
            if (last_span->font) {
                sveska_font_unref(sveska, last_span->font);
            }
            last_span->font = sveska_font_ref(new_font);
            last_span->font_size = new_font->size;
            str_ncpy(last_span->font_path, MAX_FONT_PATH_LEN, 
                    new_font->path, str_size(new_font->path));
        } else {
            // Create new span for future text
            create_new_span(sveska);
        }
    }

    // Unref old font if it was different
    if (old_font && old_font != new_font) {
        sveska_font_unref(sveska, old_font);
    }

    // Rebuild and render with new font
    rebuild_display_text(sveska);
    update_cursor_position(sveska);  // Use the new cursor position function
    sveska_text_render(sveska);
}
    

/********************************************************** */

void set_font_selected(sveska_t *sveska, const char *font_path, float size) {
    if (!sveska || !has_selection(sveska)) return;

    // Normalize selection
    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);

    // Get span positions
    size_t start_span_idx, start_span_pos;
    size_t end_span_idx, end_span_pos;
    get_span_and_pos(sveska, start, &start_span_idx, &start_span_pos);
    get_span_and_pos(sveska, end, &end_span_idx, &end_span_pos);

    // Split spans at boundaries if needed
    if (start_span_pos > 0 && !split_span_at(sveska, start_span_idx, start_span_pos)) return;
    if (start_span_pos > 0) start_span_idx++;

    text_span_t *end_span = &sveska->document.spans[end_span_idx];
    if (end_span_pos < end_span->length && !split_span_at(sveska, end_span_idx, end_span_pos)) return;

    // Store old font info for undo
    char *old_font_path = str_dup(sveska->document.spans[start_span_idx].font_path);
    float old_font_size = sveska->document.spans[start_span_idx].font_size;
    size_t span_count = end_span_idx - start_span_idx + 1;

    // Apply new font to all selected spans
    sveska_font_t *new_font = sveska_font_load(sveska, font_path, size);
    if (!new_font) {
        free(old_font_path);
        return;
    }

    for (size_t i = start_span_idx; i <= end_span_idx; i++) {
        sveska_font_unref(sveska, sveska->document.spans[i].font);
        sveska->document.spans[i].font = sveska_font_ref(new_font);
        sveska->document.spans[i].font_size = size;
        str_ncpy(sveska->document.spans[i].font_path, MAX_FONT_PATH_LEN, font_path, str_size(font_path));
    }

    // Record undo operation
    edit_operation_t op = {
        .type = OP_FORMAT,
        .data.format.span_index = start_span_idx,
        .data.format.format_type = 3, // Font change
        .data.format.font_size = size,
        .affected_span = start_span_idx,
        .timestamp = get_timestamp(),
        .user_data = malloc(MAX_FONT_PATH_LEN + sizeof(float) + sizeof(size_t))
    };

    if (op.user_data) {
        str_ncpy(op.user_data, MAX_FONT_PATH_LEN, old_font_path, str_size(old_font_path));
        *(float*)(op.user_data + MAX_FONT_PATH_LEN) = old_font_size;
        *(size_t*)(op.user_data + MAX_FONT_PATH_LEN + sizeof(float)) = span_count;
    }

    str_ncpy(op.data.format.font_path, MAX_FONT_PATH_LEN, font_path, str_size(font_path));
    record_edit(sveska, op);

    free(old_font_path);
    sveska_font_unref(sveska, new_font);
    rebuild_display_text(sveska);
}




/*********************************************************** */

void sveska_font_destroy(sveska_font_t *font) {
    if (!font || !font->is_dynamic) return;

    if (font->font_data) free(font->font_data);
    if (font->surface) free(font->surface);
    free(font);
}



/********************************************************* */

 void merge_with_previous_span(sveska_t *sveska) {
    if (sveska->current_span == 0) return; // Can't merge first span
    
    text_span_t *prev = &sveska->document.spans[sveska->current_span - 1];
    text_span_t *current = &sveska->document.spans[sveska->current_span];
    
    /* Ensure capacity */
    size_t needed = prev->length + current->length + 1;
    if (needed > prev->capacity) {
        size_t new_capacity = MAX(prev->capacity * 2, needed);
        char *new_text = realloc(prev->text, new_capacity);
        if (!new_text) {
            printf("Neuspelo spajanje spanova (neuspela realokacija)./n");
            return;
        }
        prev->text = new_text;
        prev->capacity = new_capacity;
    }
    
    /* Append text */
    memcpy(prev->text + prev->length, current->text, current->length);
    prev->length += current->length;
    prev->text[prev->length] = '\0';
    
    /* Free and remove current span */
    free(current->text);
    memmove(current, current + 1, 
           (sveska->document.count - sveska->current_span - 1) * sizeof(text_span_t));
    sveska->document.count--;
    sveska->current_span--;
}

/******************************************** */

/* Splits the current span at cursor position */
bool split_current_span(sveska_t *sveska) {
    if (!sveska || sveska->document.count >= MAX_SPANS) {
        printf("Ne mogu da podelim span - Neispravno stanje ili dostignit max broj spanova\n");
        return false;
    }

    // Ensure document has capacity for new span
    if (sveska->document.count >= sveska->document.capacity) {
        size_t new_capacity = sveska->document.capacity ? sveska->document.capacity * 2 : INITIAL_SPAN_CAPACITY;
        text_span_t *new_spans = realloc(sveska->document.spans, new_capacity * sizeof(text_span_t));
        if (!new_spans) {
            printf("Ne mogu da povecam spanove dokumenta\n");
            return false;
        }
        sveska->document.spans = new_spans;
        sveska->document.capacity = new_capacity;
    }

    text_span_t *old_span = &sveska->document.spans[sveska->current_span];
    
    // Create new span with reasonable minimum capacity
    size_t new_capacity = old_span->capacity > 0 ? old_span->capacity : 256;
    text_span_t *new_span = &sveska->document.spans[sveska->document.count++];
    
    new_span->length = 0;
    new_span->capacity = new_capacity;
    new_span->text = malloc(new_capacity);
    if (!new_span->text) {
        sveska->document.count--; // Roll back count on failure
        return false;
    }
    new_span->text[0] = '\0';
    
    new_span->font = sveska_font_ref(old_span->font); // 

    new_span->color = old_span->color;
    new_span->bold = sveska->bold;
    new_span->italic = sveska->italic;
    new_span->underline = sveska->underline;

    // Move to new span
    sveska->current_span = sveska->document.count - 1;
    
    return true;
}


/***************************************************** */

/* Determines if we need to split the current span */
bool should_split_span(sveska_t *sveska, char ch) {
    if (sveska->document.count == 0) return true;
    
    text_span_t *span = &sveska->document.spans[sveska->current_span];
    return (span->bold != sveska->bold) || 
           (span->italic != sveska->italic) ||
           (span->underline != sveska->underline);
}



/*********************************************************** */

/* Updates cursor position based on current span and text length */
void update_cursor_position(sveska_t *sveska) {
    if (!sveska || !sveska->document.spans || sveska->document.count == 0) {
        sveska->char_cursor.x = sveska->margin_x;
        sveska->char_cursor.y = sveska->text_area_offset_y;
        sveska->char_cursor.height = sveska->line_height;
        return;
    }

    // Calculate current visual line (accounts for wrapped lines)
    int visual_line = 0;
    int x = sveska->margin_x;
    int wrap_point = sveska->window_width - sveska->margin_x;
    int pos = 0;
    int prev_codepoint = 0;

    // Track the last known good y position
    int last_y = sveska->text_area_offset_y;
    
    while (pos < sveska->cursor_index) {
        if (sveska->text_buffer[pos] == '\n') {
            visual_line++;
            x = sveska->margin_x;
            prev_codepoint = 0;
            pos++;
            
            // Move to next line
            if (visual_line < (int)sveska->line_count) {
                last_y = sveska->line_y_positions[visual_line];
            } else {
                last_y += sveska->line_heights[visual_line - 1];
            }
            continue;
        }

        size_t span_idx, char_idx;
        get_span_and_pos(sveska, pos, &span_idx, &char_idx);
        if (span_idx >= sveska->document.count) {
            pos++;
            continue;
        }

        text_span_t *span = &sveska->document.spans[span_idx];
        if (!span || !span->text || char_idx >= span->length) {
            pos++;
            continue;
        }
// podrska za sliku 
if (span->is_image) {
    // Image takes up its full width and height
    x += span->img_width + sveska->char_spacing;
    
    // Update Y position based on image height
    if (span->img_height > sveska->char_cursor.height) {
        sveska->char_cursor.height = span->img_height;
    }
    
    pos++; // Treat image as one character position
    continue;
}
//kraj podrske za sliku
        sveska_font_t *font = span->font ? span->font : sveska->font;
        if (!font) {
            pos++;
            continue;
        }

        float scale = span->font_size > 0
            ? stbtt_ScaleForPixelHeight(&font->info, span->font_size)
            : font->scale;

        int codepoint = (unsigned char)span->text[char_idx];
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font->info, codepoint, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font->info, codepoint, scale, scale, &x0, &y0, &x1, &y1);

        int char_width = (x1 - x0) + (span->bold ? 1 : 0) + sveska->char_spacing;

        // Check if we need to wrap
        if (x + char_width > wrap_point && x > sveska->margin_x) {
            visual_line++;
            x = sveska->margin_x;
            prev_codepoint = 0;
            
            // Move down to next line
            if (visual_line < (int)sveska->line_count) {
                last_y = sveska->line_y_positions[visual_line];
            } else {
                last_y += sveska->line_heights[visual_line - 1];
            }
            continue; // re-process this char on new line
        }

        if (prev_codepoint) {
            x += (int)(stbtt_GetCodepointKernAdvance(&font->info, prev_codepoint, codepoint) * scale);
        }

        x += (int)(advance * scale) + sveska->char_spacing;
        prev_codepoint = codepoint;
        pos++;
    }

    sveska->char_cursor.x = x;
    sveska->last_cursor_x = x;
    
    // Set Y position based on visual line
    if (visual_line < (int)sveska->line_count) {
        sveska->char_cursor.y = sveska->line_y_positions[visual_line];
        sveska->char_cursor.height = sveska->line_heights[visual_line];
    } else {
        sveska->char_cursor.y = last_y;
        sveska->char_cursor.height = calculate_current_line_height(sveska);
    }

    // Special case: cursor at start of line after newline
    if (sveska->cursor_index > 0 && sveska->text_buffer[sveska->cursor_index - 1] == '\n') {
        sveska->char_cursor.x = sveska->margin_x;
    }
}



/******************************************************** */

/* Gets the span and position within span for a global text position */
// sveska_font.c
void get_span_and_pos(sveska_t *sveska, size_t global_pos,
    size_t *out_span_idx, size_t *out_span_pos)
{
    // Initialize outputs to safe defaults
    *out_span_idx = 0;
    *out_span_pos = 0;

    if (!sveska || !out_span_idx || !out_span_pos) return;
    if (!sveska->document.spans || sveska->document.count == 0) return;

    size_t current_global_pos = 0;
    for (size_t i = 0; i < sveska->document.count; i++) {
        text_span_t *span = &sveska->document.spans[i];
        if (!span || !span->text) continue;

        if (global_pos < current_global_pos + span->length) {
            *out_span_idx = i;
            *out_span_pos = global_pos - current_global_pos;
            return;
        }
        current_global_pos += span->length;
    }

    // Fallback to end of last span
    *out_span_idx = sveska->document.count - 1;
    *out_span_pos = sveska->document.spans[*out_span_idx].length;
}



sveska_font_t* sveska_font_ref(sveska_font_t* font) {
    if (!font) return NULL;
    font->ref_count++;
    return font;
}

void sveska_font_unref(sveska_t *sveska, sveska_font_t *font) {
    if (!font) return;
    
    font->ref_count--;
    
    // Only destroy dynamically loaded fonts
    if (font->ref_count <= 0 && font->is_dynamic) {
        sveska_font_destroy(font);
    }
}






bool sveska_config_load(sveska_t *sveska, const char *path) {
    FILE *file = fopen(path, "r");
    if (!file) {
        printf("sveska: Ne mogu da otvorim konfiguracioni fajl: %s\n", path);
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
      //  printf("Read Line: '%s'\n", line);  // Debug line content
        if (line[0] == '#' || line[0] == '\n') continue;

        char key[64], value[128];
        if (sscanf(line, "%63[^=]=%127[^\n]", key, value) != 2)
            continue;

        // Trim whitespace from key and value
        trim_whitespace(key);
        trim_whitespace(value);
         printf("Trimmed Key: '%s', Trimmed Value: '%s'\n", key, value); 
        // Apply config values to sveska_t

        if (str_cmp(key, "menu_bar_height") == 0) {
            sveska->menu_bar_height = atoi(value);
        } else if (str_cmp(key, "menu_bar_number") == 0) {
            sveska->menu_bar_number = atoi(value);
        } else if (str_cmp(key, "text_area_offset_y") == 0) {
            sveska->text_area_offset_y = atoi(value);

        } else if (str_cmp(key, "text_margin_x") == 0) {
            sveska->text_margin_x = atoi(value);
        } else if (str_cmp(key, "text_margin_y") == 0) {
            sveska->text_margin_y = atoi(value);
        } else if (str_cmp(key, "char_spacing") == 0) {
            sveska->char_spacing = atoi(value);
        } else if (str_cmp(key, "font_size") == 0) {
            int size_int = atoi(value);
            sveska->font_size = (float)size_int;
        } else if (str_cmp(key, "debug_enabled") == 0) {
            sveska->debug_enabled = atoi(value);  // 1 for enabled, 0 for disabled
            printf("Debug Enabled: %d\n", sveska->debug_enabled);  // Check value
        }
    }

    fclose(file);
    printf("Loaded configuration 0 :\n");
    // Calculate text_area_offset_y manually
   // sveska->text_area_offset_y = sveska->menu_bar_height + 10;
 //   sveska->debug_enabled = 1; 
  // Print out the loaded values for debugging
  if (sveska->debug_enabled) {
    printf("Loaded configuration 1:\n");
    printf("Menu Bar Height: %d\n", sveska->menu_bar_height);
    printf("Menu Bar Number: %d\n", sveska->menu_bar_number);
    printf("Text Margin X: %d\n", sveska->text_margin_x);
    printf("Text Margin Y: %d\n", sveska->text_margin_y);
    printf("Char Spacing: %d\n", sveska->char_spacing);
    printf("Font Size: %.2f\n", sveska->font_size);
    printf("Text Area Offset Y: %d\n", sveska->text_area_offset_y);
}
    return true;
}


/** @}
 */
