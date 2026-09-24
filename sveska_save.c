/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */

 #include "sveska.h"
 #include <stdio.h>
 #include <str.h>
 #include <errno.h>


bool save_document(sveska_t *sveska, const char *path) {
    if (!sveska || !path) return false;
    
    FILE *f = fopen(path, "w");
    if (!f) {
        printf("Ne mogu da otvorim %s za upis: %d\n", path, errno);
        return false;
    }
    
    fprintf(f, "SVESKA_DOCUMENT\n");
    fprintf(f, "VERSION=1\n");
    fprintf(f, "SPAN_COUNT=%zu\n", sveska->document.count);
    
    // Write each span
    for (size_t i = 0; i < sveska->document.count; i++) {
        text_span_t *span = &sveska->document.spans[i];
        
        fprintf(f, "\n[SPAN]\n");
        
        // Save text with newline handling
        fprintf(f, "TEXT_LEN=%zu\n", span->length);
        if (span->length > 0) {
            fwrite(span->text, 1, span->length, f);
        }
        fputc('\n', f);  // End of text marker
        
        // Save other properties
        fprintf(f, "BOLD=%d\n", span->bold);
        fprintf(f, "ITALIC=%d\n", span->italic);
        fprintf(f, "UNDERLINE=%d\n", span->underline);
        fprintf(f, "FONT=%s\n", span->font_path);
        fprintf(f, "FONT_SIZE=%.2f\n", span->font_size);
        fprintf(f, "COLOR=%04x\n", span->color);
    }
    
    fclose(f);
    
    // Update current file and modification status
    str_ncpy(sveska->current_file, MAX_PATH_LENGTH, path, str_size(path));
    sveska->document_modified = false;
    
    printf("Dokument sacuvan u %s\n", path);
    return true;
}

bool load_document(sveska_t *sveska, const char *path) {
    if (!sveska || !path) return false;
    
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Ne mogu da otvorim %s za citanje: %d\n", path, errno);
        return false;
    }
    
    char line[1024];
    int version = 0;
    size_t span_count = 0;
    size_t current_span = SIZE_MAX;
    
    // Read header
    if (!fgets(line, sizeof(line), f) || str_cmp(line, "SVESKA_DOCUMENT\n") != 0) {
        fclose(f);
        return false;
    }
    
    // Read version
    if (!fgets(line, sizeof(line), f) || sscanf(line, "VERSION=%d", &version) != 1 || version != 1) {
        fclose(f);
        return false;
    }
    
    // Read span count
    if (!fgets(line, sizeof(line), f) || sscanf(line, "SPAN_COUNT=%zu", &span_count) != 1) {
        fclose(f);
        return false;
    }
    
    // Free existing document
    if (sveska->document.spans) {
        for (size_t i = 0; i < sveska->document.count; i++) {
            text_span_t *span = &sveska->document.spans[i];
            free(span->text);
            if (span->font) {
                sveska_font_unref(sveska, span->font);
            }
        }
        free(sveska->document.spans);
    }
    
    // Initialize new document
    sveska->document.count = span_count;
    sveska->document.capacity = span_count;
    sveska->document.spans = calloc(span_count, sizeof(text_span_t));
    if (!sveska->document.spans) {
        fclose(f);
        return false;
    }
    
    // Parse spans
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        size_t len = str_length(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        if (str_cmp(line, "[SPAN]") == 0) {
            // Start new span
            current_span = (current_span == SIZE_MAX) ? 0 : current_span + 1;
            if (current_span >= span_count) break;
            
            text_span_t *span = &sveska->document.spans[current_span];
            memset(span, 0, sizeof(text_span_t));
        }
        else if (current_span < span_count) {
            text_span_t *span = &sveska->document.spans[current_span];
            
            if (str_prefix(line, "TEXT_LEN=")) {
                size_t text_len = atol(line + 9);
                if (text_len > 0) {
                    span->text = malloc(text_len + 1);
                    if (!span->text) {
                        fclose(f);
                        return false;
                    }
                    
                    // Read text content
                    size_t read = fread(span->text, 1, text_len, f);
                    if (read != text_len) {
                        free(span->text);
                        fclose(f);
                        return false;
                    }
                    span->text[text_len] = '\0';
                } else {
                    span->text = malloc(1);
                    if (span->text) {
                        span->text[0] = '\0';
                    }
                }
                span->length = text_len;
                span->capacity = text_len + 1;
                
                // Read and discard the newline after the text
                fgetc(f);
            }
            else if (str_prefix(line, "BOLD=")) {
                span->bold = atoi(line + 5);
            }
            else if (str_prefix(line, "ITALIC=")) {
                span->italic = atoi(line + 7);
            }
            else if (str_prefix(line, "UNDERLINE=")) {
                span->underline = atoi(line + 10);
            }
            else if (str_prefix(line, "FONT=")) {
                str_ncpy(span->font_path, MAX_FONT_PATH_LEN, line + 5, MAX_FONT_PATH_LEN - 1);
            }
            else if (str_prefix(line, "FONT_SIZE=")) {
                span->font_size = atof(line + 10);
            }
            else if (str_prefix(line, "COLOR=")) {
                span->color = (uint16_t)strtoul(line + 6, NULL, 16);
            }
        }
    }
    
    fclose(f);
    
    // Load fonts for each span
    for (size_t i = 0; i < span_count; i++) {
        text_span_t *span = &sveska->document.spans[i];
        
        // Skip empty spans
        if (span->length == 0) {
            // Use editor's default font for empty spans
            span->font = sveska_font_ref(sveska->font);
            span->font_size = sveska->font_size;
            str_ncpy(span->font_path, MAX_FONT_PATH_LEN, 
                    sveska->current_font_path, MAX_FONT_PATH_LEN - 1);
            continue;
        }
        
        // Try to load the specified font
        sveska_font_t *font = sveska_font_load(sveska, span->font_path, span->font_size);
        
        if (font) {
            span->font = font;
        } else {
            // Fallback to Arial if font not found
           // printf("Font not found: %s, falling back to Arial\n", span->font_path);
            font = sveska_font_load(sveska, "/fonts/arial.ttf", span->font_size);
            
            if (font) {
                span->font = font;
                str_ncpy(span->font_path, MAX_FONT_PATH_LEN, "/fonts/arial.ttf", MAX_FONT_PATH_LEN - 1);
            } else {
                // Ultimate fallback - use first available font
             //   printf("Arial not found, using first available font\n");
                if (sveska->font_count > 0) {
                    span->font = sveska_font_ref(&sveska->fonts[0]);
                    span->font_size = sveska->fonts[0].size;
                    str_ncpy(span->font_path, MAX_FONT_PATH_LEN, 
                            sveska->fonts[0].path, MAX_FONT_PATH_LEN - 1);
                }
            }
        }
    }
    
    // Set editor's current font to first span's font
    if (span_count > 0) {
        text_span_t *first_span = &sveska->document.spans[0];
        if (first_span->font) {
            // Update editor state
            sveska_font_t *old_font = sveska->font;
            sveska->font = sveska_font_ref(first_span->font);
            sveska->font_size = first_span->font_size;
            str_ncpy(sveska->current_font_path, MAX_FONT_PATH_LEN, 
                    first_span->font_path, MAX_FONT_PATH_LEN - 1);
            
            // Release old font if different
            if (old_font && old_font != sveska->font) {
                sveska_font_unref(sveska, old_font);
            }
        }
        
        // Set current style to first span's style
        sveska->bold = first_span->bold;
        sveska->italic = first_span->italic;
        sveska->underline = first_span->underline;
    }
    
    // Set current file and reset modification flag
    str_ncpy(sveska->current_file, MAX_PATH_LENGTH, path, MAX_PATH_LENGTH - 1);
    sveska->document_modified = false;
    
    // Reset cursor and selection
    sveska->cursor_index = 0;
    sveska->current_span = 0;
    sveska->selection.start_pos = -1;
    sveska->selection.end_pos = -1;
    
    // Rebuild display text and render
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
    
    printf("Dokument ucitan iz %s\n", path);
    return true;
}



/**
 * Import a raw text file into the document
 * 
 * @param sveska Editor state
 * @param path Path to the text file
 * @return true on success, false on failure
 */
 bool open_txt_file(sveska_t *sveska, const char *path)
 {
    if (!sveska || !path) return false;
    
    FILE *f = fopen(path, "r");
    if (!f) {
        printf("Ne mogu da otvorim %s za citanje: %d\n", path, errno);
        return false;
    }
    
    // Count lines first to determine span count
    size_t line_count = 0;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), f)) {
        line_count++;
    }
    rewind(f);
    
    // Free existing document
    if (sveska->document.spans) {
        for (size_t i = 0; i < sveska->document.count; i++) {
            text_span_t *span = &sveska->document.spans[i];
            free(span->text);
            if (span->font) {
                sveska_font_unref(sveska, span->font);
            }
        }
        free(sveska->document.spans);
    }
    
    // Create new document structure
    sveska->document.count = line_count;
    sveska->document.capacity = line_count;
    sveska->document.spans = calloc(line_count, sizeof(text_span_t));
    if (!sveska->document.spans) {
        fclose(f);
        return false;
    }
    
    // Find or load Arial font
    sveska_font_t *arial_font = NULL;
    for (size_t i = 0; i < sveska->font_count; i++) {
        if (str_casecmp(sveska->font_names[i], "arial") == 0 || 
            str_casecmp(sveska->font_names[i], "arial.ttf") == 0) {
            arial_font = &sveska->fonts[i];
            break;
        }
    }
    
    // Fallback to first font if Arial not found
    if (!arial_font && sveska->font_count > 0) {
        arial_font = &sveska->fonts[0];
    }
    
    // Default style parameters
    const float default_font_size = 16.0f;
    const uint16_t default_color = 0x0000; // Black
    
    // Process each line
    for (size_t i = 0; i < line_count; i++) {
        if (!fgets(buffer, sizeof(buffer), f)) break;
        
        text_span_t *span = &sveska->document.spans[i];
        
        // Calculate text length including newline
        size_t len = str_length(buffer);
        
        // Check if we have a newline at the end
        bool has_newline = (len > 0 && buffer[len-1] == '\n');
        
        // If we have CRLF, remove the CR
        if (has_newline && len > 1 && buffer[len-2] == '\r') {
            buffer[len-2] = '\n';  // Replace CR with LF
            buffer[len-1] = '\0';  // Remove extra character
            len--;
        }
        
        // Allocate memory for text + newline if needed
        size_t alloc_len = len;
        if (!has_newline) {
            alloc_len++;  // Add space for newline
        }
        
        span->text = malloc(alloc_len + 1);
        if (!span->text) {
            // Clean up partially created document
            for (size_t j = 0; j < i; j++) {
                free(sveska->document.spans[j].text);
            }
            free(sveska->document.spans);
            sveska->document.spans = NULL;
            sveska->document.count = 0;
            sveska->document.capacity = 0;
            fclose(f);
            return false;
        }
        
        // Copy text content
        if (len > 0) {
            str_ncpy(span->text, alloc_len + 1, buffer, len);
        }
        
        // Add newline if not present
        if (!has_newline) {
            span->text[len] = '\n';
            len++;
        }
        
        span->text[len] = '\0';
        span->length = len;
        span->capacity = len + 1;
        
        // Apply default formatting
        span->bold = false;
        span->italic = false;
        span->underline = false;
        span->font_size = default_font_size;
        span->color = default_color;
        
        if (arial_font) {
            span->font = sveska_font_ref(arial_font);
            str_ncpy(span->font_path, MAX_FONT_PATH_LEN, 
                    arial_font->path, MAX_FONT_PATH_LEN - 1);
        }
    }
    
    fclose(f);
    
    // Update editor state
    if (line_count > 0) {
        // Set editor's current font to first span's font
        text_span_t *first_span = &sveska->document.spans[0];
        if (first_span->font) {
            // Update editor state
            sveska_font_t *old_font = sveska->font;
            sveska->font = sveska_font_ref(first_span->font);
            sveska->font_size = first_span->font_size;
            str_ncpy(sveska->current_font_path, MAX_FONT_PATH_LEN, 
                    first_span->font_path, MAX_FONT_PATH_LEN - 1);
            
            // Release old font if different
            if (old_font && old_font != sveska->font) {
                sveska_font_unref(sveska, old_font);
            }
        }
    }
    
    // Set current file and reset modification flag
    str_ncpy(sveska->current_file, MAX_PATH_LENGTH, path, MAX_PATH_LENGTH - 1);
    sveska->document_modified = false;
    
    // Reset cursor and selection
    sveska->cursor_index = 0;
    sveska->current_span = 0;
    sveska->selection.start_pos = -1;
    sveska->selection.end_pos = -1;
    
    // Rebuild display text and render
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
    
    printf("Tekstualni fajl uvezen iz %s\n", path);
    return true;
}
 



/** @}
 */