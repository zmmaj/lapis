

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */


#include "sveska.h"
#include <str.h>
#include <math.h>

void sveska_toggle_bold(sveska_t *sveska) {
    if (!sveska) return;

    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
        // No selection - toggle bold for new text
        sveska->bold = !sveska->bold;
    } else {
        // Apply to selection
        toggle_bold_selected(sveska);
    }
}

void toggle_bold_selected(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("No valid selection to bold\n");
        return;
    }

    /* Get normalized selection range */
    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);
    
    /* Get document length and validate selection */
    size_t doc_length = get_document_length(sveska);
    if (doc_length == 0) {
        printf("Empty document - nothing to bold\n");
        return;
    }
    
    start = CLAMP(start, 0, (int)doc_length - 1);
    end = CLAMP(end, 0, (int)doc_length - 1);
    if (start >= end) {
        printf("Invalid selection range\n");
        return;
    }

    /* Get span positions for selection boundaries */
    size_t start_span_idx, start_span_pos;
    size_t end_span_idx, end_span_pos;
    get_span_and_pos(sveska, start, &start_span_idx, &start_span_pos);
    get_span_and_pos(sveska, end, &end_span_idx, &end_span_pos);

    printf("Selection spans: [%zu:%zu] to [%zu:%zu]\n",
          start_span_idx, start_span_pos, end_span_idx, end_span_pos);

    /* Special case: single span selection */
    if (start_span_idx == end_span_idx) {
        text_span_t *span = &sveska->document.spans[start_span_idx];
        
        /* If selection covers entire span, just toggle it */
        if (start_span_pos == 0 && end_span_pos == span->length) {
            printf("Toggling bold for entire span %zu\n", start_span_idx);
            
            // Record the operation BEFORE toggling
            edit_operation_t op = {
                .type = OP_FORMAT,
                .data.format.span_index = start_span_idx,
                .data.format.old_value = span->bold,
                .data.format.new_value = !span->bold,
                .data.format.format_type = FORMAT_BOLD,
                .affected_span = start_span_idx,
                .timestamp = get_timestamp()
            };
            record_edit(sveska, op);
            
            span->bold = !span->bold;
            rebuild_display_text(sveska);
            update_cursor_position(sveska);
            sveska_text_render(sveska);
            return;
        }
    }

    /* Split at selection start if needed */
    if (start_span_pos > 0) {
        printf("Splitting at start: span %zu, pos %zu\n", start_span_idx, start_span_pos);
        if (!split_span_at(sveska, start_span_idx, start_span_pos)) {
            printf("Failed to split at start\n");
            return;
        }
        /* After split, our target is now the new span */
        start_span_idx++;
        start_span_pos = 0;
        
        /* Adjust end position if it was in the same span */
        if (end_span_idx == start_span_idx - 1) {
            end_span_idx = start_span_idx;
            end_span_pos -= sveska->document.spans[start_span_idx-1].length;
        }
    }

    /* Split at selection end if needed */
    text_span_t *end_span = &sveska->document.spans[end_span_idx];
    if (end_span_pos < end_span->length) {
        printf("Splitting at end: span %zu, pos %zu\n", end_span_idx, end_span_pos);
        if (!split_span_at(sveska, end_span_idx, end_span_pos)) {
            printf("Failed to split at end\n");
            return;
        }
    }

    /* Record the operation BEFORE making changes */
    edit_operation_t op = {
        .type = OP_FORMAT,
        .data.format.span_index = start_span_idx,
        .data.format.old_value = !sveska->bold, // Current state before toggle
        .data.format.new_value = sveska->bold,  // State after toggle
        .data.format.format_type = FORMAT_BOLD,
        .affected_span = start_span_idx,
        .timestamp = get_timestamp()
    };
    record_edit(sveska, op);

    /* Toggle bold for all fully selected spans */
    for (size_t i = start_span_idx; i <= end_span_idx && i < sveska->document.count; i++) {
        printf("Toggling bold for span %zu (len %zu, was %d)\n", 
              i, sveska->document.spans[i].length, sveska->document.spans[i].bold);
        sveska->document.spans[i].bold = !sveska->document.spans[i].bold;
    }

    sveska->document_modified = true;
    /* Update display */
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}


/****************************************************** */


void sveska_toggle_italic(sveska_t *sveska) {
    if (!sveska) return;

    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
        // No selection - toggle bold for new text
        sveska->bold = !sveska->bold;
    } else {
        // Apply to selection
        toggle_italic_selected(sveska);
    }
}


void toggle_italic_selected(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("No valid selection to italicize\n");
        return;
    }

    /* Get normalized selection range */
    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);
    
    /* Get document length and validate selection */
    size_t doc_length = get_document_length(sveska);
    if (doc_length == 0) {
        printf("Empty document - nothing to italicize\n");
        return;
    }
    
    start = CLAMP(start, 0, (int)doc_length - 1);
    end = CLAMP(end, 0, (int)doc_length - 1);
    if (start >= end) {
        printf("Invalid selection range\n");
        return;
    }

    printf("Toggling italic for selection %d-%d (doc len: %zu)\n", start, end, doc_length);

    /* Get span positions for selection boundaries */
    size_t start_span_idx, start_span_pos;
    size_t end_span_idx, end_span_pos;
    get_span_and_pos(sveska, start, &start_span_idx, &start_span_pos);
    get_span_and_pos(sveska, end, &end_span_idx, &end_span_pos);

    printf("Selection spans: [%zu:%zu] to [%zu:%zu]\n",
          start_span_idx, start_span_pos, end_span_idx, end_span_pos);

    /* Special case: single span selection */
    if (start_span_idx == end_span_idx) {
        text_span_t *span = &sveska->document.spans[start_span_idx];
        
        /* If selection covers entire span, just toggle it */
        if (start_span_pos == 0 && end_span_pos == span->length) {
            printf("Toggling italic for entire span %zu\n", start_span_idx);
            span->italic = !span->italic;
            rebuild_display_text(sveska);
            sveska_text_render(sveska);
            return;
        }
    }

    /* Split at selection start if needed */
    if (start_span_pos > 0) {
        printf("Splitting at start: span %zu, pos %zu\n", start_span_idx, start_span_pos);
        if (!split_span_at(sveska, start_span_idx, start_span_pos)) {
            printf("Failed to split at start\n");
            return;
        }
        /* After split, our target is now the new span */
        start_span_idx++;
        start_span_pos = 0;
        
        /* Adjust end position if it was in the same span */
        if (end_span_idx == start_span_idx - 1) {
            end_span_idx = start_span_idx;
            end_span_pos -= sveska->document.spans[start_span_idx-1].length;
        }
    }

    /* Split at selection end if needed */
    text_span_t *end_span = &sveska->document.spans[end_span_idx];
    if (end_span_pos < end_span->length) {
        printf("Splitting at end: span %zu, pos %zu\n", end_span_idx, end_span_pos);
        if (!split_span_at(sveska, end_span_idx, end_span_pos)) {
            printf("Failed to split at end\n");
            return;
        }
    }

    edit_operation_t op = {
        .type = OP_FORMAT,
        .data.format.span_index = start_span_idx,
        .data.format.old_value = !sveska->italic, // or detect actual previous value if needed
        .data.format.new_value = sveska->italic,
        .data.format.format_type = FORMAT_ITALIC,
        .affected_span = start_span_idx,
        .timestamp = get_timestamp()
    };
    record_edit(sveska, op);

    /* Toggle italic for all fully selected spans */
    for (size_t i = start_span_idx; i <= end_span_idx && i < sveska->document.count; i++) {
        printf("Toggling italic for span %zu (len %zu, was %d)\n", 
              i, sveska->document.spans[i].length, sveska->document.spans[i].italic);
        sveska->document.spans[i].italic = !sveska->document.spans[i].italic;
    }
    sveska->document_modified = true;
    /* Update display */
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}


/****************************************************** */

void sveska_toggle_underline(sveska_t *sveska) {
    if (!sveska) return;

    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
        // No selection - toggle bold for new text
        sveska->bold = !sveska->bold;
    } else {
        // Apply to selection
        toggle_underline_selected(sveska);
    }
}

void toggle_underline_selected(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("No valid selection to underline\n");
        return;
    }

    /* Get normalized selection range */
    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);
    
    /* Get document length and validate selection */
    size_t doc_length = get_document_length(sveska);
    if (doc_length == 0) {
        printf("Empty document - nothing to underline\n");
        return;
    }
    
    start = CLAMP(start, 0, (int)doc_length - 1);
    end = CLAMP(end, 0, (int)doc_length - 1);
    if (start >= end) {
        printf("Invalid selection range\n");
        return;
    }

   // printf("Toggling underline for selection %d-%d (doc len: %zu)\n", start, end, doc_length);

    /* Get span positions for selection boundaries */
    size_t start_span_idx, start_span_pos;
    size_t end_span_idx, end_span_pos;
    get_span_and_pos(sveska, start, &start_span_idx, &start_span_pos);
    get_span_and_pos(sveska, end, &end_span_idx, &end_span_pos);

 //   printf("Selection spans: [%zu:%zu] to [%zu:%zu]\n",
 //         start_span_idx, start_span_pos, end_span_idx, end_span_pos);

    /* Special case: single span selection */
    if (start_span_idx == end_span_idx) {
        text_span_t *span = &sveska->document.spans[start_span_idx];
        
        /* If selection covers entire span, just toggle it */
        if (start_span_pos == 0 && end_span_pos == span->length) {
           // printf("Toggling underline for entire span %zu\n", start_span_idx);
           sveska->document_modified = true;
           span->underline = !span->underline;
            rebuild_display_text(sveska);
            sveska_text_render(sveska);
            return;
        }
    }

    /* Split at selection start if needed */
    if (start_span_pos > 0) {
        printf("Splitting at start: span %zu, pos %zu\n", start_span_idx, start_span_pos);
        if (!split_span_at(sveska, start_span_idx, start_span_pos)) {
            printf("Failed to split at start\n");
            return;
        }
        /* After split, our target is now the new span */
        start_span_idx++;
        start_span_pos = 0;
        
        /* Adjust end position if it was in the same span */
        if (end_span_idx == start_span_idx - 1) {
            end_span_idx = start_span_idx;
            end_span_pos -= sveska->document.spans[start_span_idx-1].length;
        }
    }

    /* Split at selection end if needed */
    text_span_t *end_span = &sveska->document.spans[end_span_idx];
    if (end_span_pos < end_span->length) {
        printf("Splitting at end: span %zu, pos %zu\n", end_span_idx, end_span_pos);
        if (!split_span_at(sveska, end_span_idx, end_span_pos)) {
            printf("Failed to split at end\n");
            return;
        }
    }
    edit_operation_t op = {
        .type = OP_FORMAT,
        .data.format.span_index = start_span_idx,
        .data.format.old_value = !sveska->underline, // or the previous underline value
        .data.format.new_value = sveska->underline,
        .data.format.format_type = FORMAT_UNDERLINE,
        .affected_span = start_span_idx,
        .timestamp = get_timestamp()
    };
    record_edit(sveska, op);

    /* Toggle underline for all fully selected spans */
    for (size_t i = start_span_idx; i <= end_span_idx && i < sveska->document.count; i++) {
        printf("Toggling underline for span %zu (len %zu, was %d)\n", 
              i, sveska->document.spans[i].length, sveska->document.spans[i].underline);
        sveska->document.spans[i].underline = !sveska->document.spans[i].underline;
    }
    sveska->document_modified = true;
    /* Update display */
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}

/*************************************************** */

bool split_span_at(sveska_t *sveska, size_t span_idx, size_t split_pos) {
    if (!sveska || span_idx >= sveska->document.count || 
        split_pos > sveska->document.spans[span_idx].length) {
        printf("Invalid split parameters\n");
        return false;
    }

        // Get the span pointer
        text_span_t *span = &sveska->document.spans[span_idx];
    
        // Check if it's an image span
        if (span->is_image) {
            printf("Cannot split image spans\n");
            return false;
        }

    // Early exit if split position is at boundary
    if (split_pos == 0 || split_pos == sveska->document.spans[span_idx].length) {
        return true;  // No-op but technically successful
    }

    // Ensure capacity
    if (sveska->document.count >= sveska->document.capacity) {
        size_t new_capacity = sveska->document.capacity * 2;
        text_span_t *new_spans = realloc(sveska->document.spans, 
                                       new_capacity * sizeof(text_span_t));
        if (!new_spans) {
            printf("Failed to expand document capacity\n");
            return false;
        }
        sveska->document.spans = new_spans;
        sveska->document.capacity = new_capacity;
    }

    text_span_t *old_span = &sveska->document.spans[span_idx];
    
    // Create new span for the right portion
    text_span_t new_span = {
        .length = old_span->length - split_pos,
        .capacity = old_span->capacity,
        .text = malloc(old_span->capacity),
        .font = sveska_font_ref(old_span->font),  // Ref-count font!
        .color = old_span->color,
        .bold = old_span->bold,
        .italic = old_span->italic,
        .underline = old_span->underline,
        .font_size = old_span->font_size  // Critical for rendering!
    };
    
    if (!new_span.text) {
        printf("Ne mogu da alociram new span text\n");
        return false;
    }

    // Copy text to new span
    memcpy(new_span.text, old_span->text + split_pos, new_span.length);
    new_span.text[new_span.length] = '\0';  // Ensure null-termination
    
    // Truncate old span
    old_span->length = split_pos;
    old_span->text[old_span->length] = '\0';
    
    // Insert the new span after the old one
    memmove(&sveska->document.spans[span_idx+2], 
            &sveska->document.spans[span_idx+1],
            (sveska->document.count - span_idx - 1) * sizeof(text_span_t));
    
    sveska->document.spans[span_idx+1] = new_span;
    sveska->document.count++;
    
    return true;
}

/************************************************************ */

void delete_selected_text(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("No valid selection to delete\n");
        return;
    }

    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    //int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);

    // 1. Validate and clamp selection range
    int doc_length = get_document_length(sveska);
    int sel_start = CLAMP(MIN(sveska->selection.start_pos, sveska->selection.end_pos), 0, doc_length);
    int sel_end = CLAMP(MAX(sveska->selection.start_pos, sveska->selection.end_pos), 0, doc_length);

    if (sel_start >= sel_end) {
        printf("Invalid selection range\n");
        return;
    }
    size_t sel_length = sel_end - sel_start;

    // 2. Create persistent copy of selected text with styles
    clipboard_data_t *selected_data = malloc(sizeof(clipboard_data_t));
    if (selected_data && sel_length > 0) {
        selected_data->text = str_ndup(sveska->text_buffer + sel_start, sel_length);
        selected_data->bold = malloc(sel_length);
        selected_data->italic = malloc(sel_length);
        selected_data->underline = malloc(sel_length);
        selected_data->length = sel_length;

        if (selected_data->text && selected_data->bold && 
            selected_data->italic && selected_data->underline) {
            
            // Get font info from first span
            size_t first_span_idx, dummy_pos;
            get_span_and_pos(sveska, sel_start, &first_span_idx, &dummy_pos);
            text_span_t *first_span = &sveska->document.spans[first_span_idx];
            
            // Store font info
            if (first_span->font) {
                str_ncpy(selected_data->font_path, MAX_FONT_PATH_LEN, 
                        first_span->font_path, str_size(first_span->font_path));
                selected_data->font_size = first_span->font_size;
            } else {
                // Fallback to current font if span has none
                str_ncpy(selected_data->font_path, MAX_FONT_PATH_LEN, 
                        sveska->current_font_path, str_size(sveska->current_font_path));
                selected_data->font_size = sveska->font_size;
            }

            // Capture style information for each character
            for (int i = 0; i < (int)sel_length; i++) {
                size_t global_pos = sel_start + i;
                size_t span_idx, span_pos;
                get_span_and_pos(sveska, global_pos, &span_idx, &span_pos);
                text_span_t *span = &sveska->document.spans[span_idx];
                
                selected_data->bold[i] = span->bold;
                selected_data->italic[i] = span->italic;
                selected_data->underline[i] = span->underline;
            }
        } else {
            clipboard_free_data(selected_data);
            selected_data = NULL;
        }
    }

    // 3. Determine affected span
    size_t affected_span, dummy_pos;
    get_span_and_pos(sveska, sel_start, &affected_span, &dummy_pos);

    // 4. Record operation before modifying document
    edit_operation_t op = {
        .type = OP_DELETE,
        .data.text.text = selected_data ? selected_data->text : NULL,
        .data.text.position = sel_start,
        .data.text.length = sel_length,
        .affected_span = affected_span,
        .timestamp = get_timestamp(),
        .user_data = selected_data  // Store the full style information
    };
    record_edit(sveska, op);

    // 5. Perform the actual deletion in document spans
    size_t start_span_idx, start_pos;
    size_t end_span_idx, end_pos;
    get_span_and_pos(sveska, sel_start, &start_span_idx, &start_pos);
    get_span_and_pos(sveska, sel_end, &end_span_idx, &end_pos);

    text_span_t *start_span = &sveska->document.spans[start_span_idx];
    text_span_t *end_span = &sveska->document.spans[end_span_idx];

    if (start_span_idx == end_span_idx) {
        // Single-span deletion
        size_t delete_len = end_pos - start_pos;
        memmove(&start_span->text[start_pos], 
               &start_span->text[end_pos], 
               start_span->length - end_pos + 1);
        start_span->length -= delete_len;
    } else {
        // Cross-span deletion
        start_span->text[start_pos] = '\0';
        start_span->length = start_pos;

        // Append remaining text from end span if needed
        size_t end_remaining = end_span->length - end_pos;
        if (end_remaining > 0) {
            size_t new_len = start_span->length + end_remaining;
            char *new_text = realloc(start_span->text, new_len + 1);
            if (new_text) {
                start_span->text = new_text;
                memcpy(&start_span->text[start_span->length], 
                      &end_span->text[end_pos], 
                      end_remaining);
                start_span->length = new_len;
                start_span->text[new_len] = '\0';
            }
        }

        // Free and remove intermediate spans
        for (size_t i = start_span_idx + 1; i <= end_span_idx; i++) {
            if (sveska->document.spans[i].text) {
                free(sveska->document.spans[i].text);
            }
        }
        memmove(&sveska->document.spans[start_span_idx + 1],
               &sveska->document.spans[end_span_idx + 1],
               (sveska->document.count - end_span_idx - 1) * sizeof(text_span_t));
        sveska->document.count -= (end_span_idx - start_span_idx);
    }

    // 6. Update document state
    sveska->current_span = start_span_idx;


       // NEW: Set cursor to END of selection after deletion
       sveska->cursor_index = start;  // Not end, because text was removed
       sveska->selection.start_pos = -1;
       sveska->selection.end_pos = -1;

       sveska->document_modified = true;
    // 7. Refresh display
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}


/******************************************** */

void record_edit(sveska_t *sveska, edit_operation_t op) {
    if (!sveska || sveska->in_undo_redo) return;

        // Capture current font for text operations
    // For insert operations, capture current font
    if (op.type == OP_INSERT) {
        text_span_t *span = get_current_span(sveska);
        if (span && span->font) {
            str_ncpy(op.data.text.font_path, MAX_FONT_PATH_LEN, 
                    span->font->path, str_size(span->font->path));
            op.data.text.font_size = span->font_size;
        }
    }

    // Free any operations we're overwriting in redo history
    if (sveska->history_pos < sveska->history_size) {
        for (size_t i = sveska->history_pos; i < sveska->history_size; i++) {
            edit_operation_t *old_op = &sveska->edit_history[i];
            if ((old_op->type == OP_INSERT || old_op->type == OP_DELETE) &&
                old_op->data.text.text) {
                printf("Freeing overwritten text: %p\n", old_op->data.text.text);
                free(old_op->data.text.text);
                old_op->data.text.text = NULL;
            }
        }
        sveska->history_size = sveska->history_pos;
    }

    // Ensure capacity
    if (sveska->history_size >= sveska->history_capacity) {
        size_t new_capacity = sveska->history_capacity ? sveska->history_capacity * 2 : 16;
        edit_operation_t *new_history = realloc(sveska->edit_history, 
                                             new_capacity * sizeof(edit_operation_t));
        if (!new_history) {
            if (op.data.text.text) free(op.data.text.text);
            return;
        }
        sveska->edit_history = new_history;
        sveska->history_capacity = new_capacity;
    }

    // Store the operation
    sveska->edit_history[sveska->history_size++] = op;
    sveska->history_pos = sveska->history_size;
    
}


// Helper to apply an operation
void apply_operation(sveska_t *sveska, edit_operation_t *op, bool undo) {
    if (!sveska || !op) {
        printf("Invalid parameters to apply_operation\n");
        return;
    }
    
    printf("Entering apply_operation (undo=%d)\n", undo);

    // Deep validation of document state
    if (sveska->document.count == 0 || !sveska->document.spans) {
        printf("Document empty or spans NULL, initializing...\n");
        if (!initialize_document(sveska)) {
            printf("CRITICAL: Failed to initialize document\n");
            return;
        }
    }

    // Validate all spans
    for (size_t i = 0; i < sveska->document.count; i++) {
        if (!sveska->document.spans[i].text) {
            printf("CRITICAL: Span %zu has NULL text pointer! Reinitializing...\n", i);
            if (!initialize_document(sveska)) {
                printf("CRITICAL: Failed to reinitialize document\n");
                return;
            }
            break;
        }
    }

    //printf("Document validated: count=%zu\n", sveska->document.count);

    // Validate operation
    if (op->affected_span >= sveska->document.count) {
        printf("Adjusting invalid affected span %zu to %zu\n",
              op->affected_span, sveska->document.count - 1);
        op->affected_span = sveska->document.count - 1;
    }

    sveska->in_undo_redo = true;
  
    switch (op->type) {
        case OP_INSERT: {
            if (!undo && op->data.text.font_path[0]) {
                // Redo: set font before inserting
                sveska_set_font(sveska, op->data.text.font_path, op->data.text.font_size);
            }
        
            // Position validation
            size_t doc_length = get_document_length(sveska);
            if (op->data.text.position > doc_length) {
                printf("Neispravna pozicija insertovanja %zu > %zu, klampujem\n",
                       op->data.text.position, doc_length);
                op->data.text.position = doc_length;
            }
        
            if (undo) {
                // Undo: delete the inserted text
                size_t pos = op->data.text.position;
                size_t len = op->data.text.length;
        
                // Move cursor to deletion point
                sveska->cursor_index = pos;
                size_t dummy_pos;
                get_span_and_pos(sveska, pos, &sveska->current_span, &dummy_pos);
        
                if (len > 0) {
                    sveska->selection.start_pos = pos;
                    sveska->selection.end_pos = pos + len;
                    delete_selected_text(sveska);
                }
        
                // NOTE: If you want to restore the font before the insert, you would do it here.
                // But as you said, don't invent fields like prev_font_path, so we skip that.
        
            } else {
                // Redo: direct text insertion
                if (op->data.text.text && op->data.text.length > 0) {
                    // 1. Force cursor to original position
                    sveska->cursor_index = op->data.text.position;
        
                    // 2. Get current span context
                    size_t span_idx, pos_in_span;
                    get_span_and_pos(sveska, op->data.text.position, &span_idx, &pos_in_span);
                    sveska->current_span = span_idx;
        
                    // 3. Ensure span exists
                    while (span_idx >= sveska->document.count) {
                        if (!create_new_span(sveska)) break;
                        span_idx = sveska->document.count - 1;
                    }
        
                    text_span_t *span = &sveska->document.spans[span_idx];
        
                    // 4. Ensure capacity
                    while (span->length + op->data.text.length >= span->capacity) {
                        size_t new_cap = span->capacity * 2;
                        char *new_text = realloc(span->text, new_cap);
                        if (!new_text) break;
                        span->text = new_text;
                        span->capacity = new_cap;
                    }
        
                    // 5. Insert text directly
                    memmove(span->text + pos_in_span + op->data.text.length,
                            span->text + pos_in_span,
                            span->length - pos_in_span);
                    memcpy(span->text + pos_in_span,
                           op->data.text.text,
                           op->data.text.length);
        
                    span->length += op->data.text.length;
                    sveska->cursor_index += op->data.text.length;
        
                    // 6. Update UI
                    rebuild_display_text(sveska);
                    update_cursor_position(sveska);
                }
            }
        
            break;
        }
        
       
        case OP_DELETE: {
            printf("aply delete 0).\n");
            if ((int)op->data.text.position > get_document_length(sveska)) {
                printf("Neispravna pozicija brisanja %zu\n", op->data.text.position);
                break;
            }

            if (undo) {
               // printf("Dodajem UNDO za DELETE operaciju\n");
                if (op->data.text.text && op->data.text.length > 0) {
                  //  printf("Restoring deleted text: '%.*s'\n", 
                   //       (int)op->data.text.length, op->data.text.text);
                    
                    // Save current state
                    bool was_pasting = sveska->paste;
                    sveska->paste = true; // Prevent recording during undo
                    
                    // Move cursor to correct position
                    sveska->cursor_index = op->data.text.position;
                    size_t span_idx, pos_in_span;
                    get_span_and_pos(sveska, op->data.text.position, &span_idx, &pos_in_span);
                    sveska->current_span = span_idx;
                    
                    // Insert each character individually
                    for (size_t i = 0; i < op->data.text.length; i++) {
                        char ch = op->data.text.text[i];
                       // printf("Restoring char '%c' at pos %zu\n", ch, op->data.text.position + i);
                        
                        // Get current span for this position
                        get_span_and_pos(sveska, op->data.text.position + i, &span_idx, &pos_in_span);
                        sveska->current_span = span_idx;
                        text_span_t *span = &sveska->document.spans[span_idx];
                        
                        // Ensure span capacity
                        if (span->length >= span->capacity - 1) {
                            size_t new_cap = span->capacity * 2;
                            char *new_text = realloc(span->text, new_cap);
                            if (!new_text) break;
                            span->text = new_text;
                            span->capacity = new_cap;
                        }
                        
                        // Insert character
                        if (pos_in_span > span->length) pos_in_span = span->length;
                        memmove(&span->text[pos_in_span + 1], &span->text[pos_in_span], 
                               span->length - pos_in_span);
                        span->text[pos_in_span] = ch;
                        span->length++;
                        span->text[span->length] = '\0';
                        
                        sveska->cursor_index++;
                    }
                    
                    sveska->paste = was_pasting; // Restore state
                }
            }
             else {
              
                // For redo, delete the text again
                if (op->data.text.length > 0) {
                    size_t pos = op->data.text.position;
                    size_t len = op->data.text.length;
                    
                    // Select the text
                    sveska->selection.start_pos = pos;
                    sveska->selection.end_pos = pos + len;
                    
                    // Delete the selection
                    delete_selected_text(sveska);
                }
            }
            break;
        }
            
        case OP_FORMAT: {
            // Validate affected span
            if (op->affected_span >= sveska->document.count) {
                printf("Neispravan index spana %zu >= %zu\n",
                      op->affected_span, sveska->document.count);
                break;
            }
        
            text_span_t *span = &sveska->document.spans[op->affected_span];
            bool new_value = undo ? op->data.format.old_value : op->data.format.new_value;
            
            switch (op->data.format.format_type) {
                case FORMAT_BOLD:
                    span->bold = new_value;
                  //  printf("Applied %s to span %zu (bold=%d)\n", undo ? "UNDO" : "REDO", op->affected_span, span->bold);
                    break;
                    
                case FORMAT_ITALIC:
                    span->italic = new_value;
                 //   printf("Applied %s to span %zu (italic=%d)\n",
                  //        undo ? "UNDO" : "REDO", op->affected_span, span->italic);
                    break;
                    
                case FORMAT_UNDERLINE:
                    span->underline = new_value;
                 //   printf("Applied %s to span %zu (underline=%d)\n",
                //          undo ? "UNDO" : "REDO", op->affected_span, span->underline);
                    break;
                    
                case 3: {  // Font change
                    const char *font_path;
                    float font_size;
                    size_t span_count;
                    
                    if (undo) {
                        // Get old font info from user_data
                        font_path = op->user_data;
                        font_size = *(float*)(op->user_data + MAX_FONT_PATH_LEN);
                        span_count = *(size_t*)(op->user_data + MAX_FONT_PATH_LEN + sizeof(float));
                   //     printf("Undoing font change to %s (%.1fpt) on %zu spans\n",
                      //        font_path, font_size, span_count);
                    } else {
                        // Use new font info from operation
                        font_path = op->data.format.font_path;
                        font_size = op->data.format.font_size;
                        span_count = *(size_t*)(op->user_data + MAX_FONT_PATH_LEN + sizeof(float));
                     //   printf("Redoing font change to %s (%.1fpt) on %zu spans\n",
                    //          font_path, font_size, span_count);
                    }
        
                    // Apply to all affected spans
                    size_t end_span = MIN(op->affected_span + span_count, sveska->document.count);
                    for (size_t i = op->affected_span; i < end_span; i++) {
                        text_span_t *span = &sveska->document.spans[i];
                        sveska_font_t *old_font = span->font;
                        
                        span->font = sveska_font_load(sveska, font_path, font_size);
                        if (!span->font) {
                            printf("Neuspelo ucitavanje fonta %s (%.1fpt)\n", font_path, font_size);
                            continue;
                        }
                        
                        span->font_size = font_size;
                        str_ncpy(span->font_path, MAX_FONT_PATH_LEN, font_path, str_size(font_path));
                        
                        if (old_font) {
                            sveska_font_unref(sveska, old_font);
                        }
                      //  printf("Applied font to span %zu: %s (%.1fpt)\n",
                       //       i, span->font_path, span->font_size);
                    }
                    break;
                }
                    
                default:
                    printf("Nepoznat tip formata: %d\n", op->data.format.format_type);
                    break;
            }
            break;
        }
  
        default:
            printf("Nepoznat tip operacije: %d\n", op->type);
            break;
    }

    sveska->in_undo_redo = false;
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}

// sveska_edit.c
void insert_char_with_current_style(sveska_t *sveska, char ch) {
    if (!sveska) {
        printf("insert_char: Neispravna instanca sveske\n");
        return;
    }
    
  //  printf("Inserting char '%c' at cursor index %u\n", ch, sveska->cursor_index);

    // Validate document state
    if (!sveska->document.spans || sveska->document.count == 0) {
        printf("insert_char: Dokument nije iniciran\n");
        return;
    }

    if (has_selection(sveska)) {
      //  printf("Deleting selected text before insertion\n");
        delete_selected_text(sveska);
    }

    // Get current span and position with validation
    size_t span_idx, pos_in_span;
    get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
    
   // printf("Current span: %zu, pos in span: %zu\n", span_idx, pos_in_span);
    
    if (span_idx >= sveska->document.count) {
        printf("GRESKA: Neispravan indx spana %zu (max %zu)\n", 
               span_idx, sveska->document.count - 1);
        return;
    }
    
    text_span_t *span = &sveska->document.spans[span_idx];
    if (!span) {
        printf("GRESKA: Span %zu je NULL\n", span_idx);
        return;
    }
    
    // Ensure text buffer exists
    if (!span->text) {
        printf("POZOR: Span %zu ima prazan text bafer. Kreiram novi buffer...\n", span_idx);
        span->capacity = 256;
        span->text = malloc(span->capacity);
        if (!span->text) {
            printf("GRESKA: Ne mogu da alociram bafer texta\n");
            return;
        }
        span->length = 0;
        span->text[0] = '\0';
    }
    
    // Validate position within span
    if (pos_in_span > span->length) {
     //   printf("WARNING: Adjusting position %zu to %zu (span length)\n", 
       //        pos_in_span, span->length);
        pos_in_span = span->length;
    }

    // Check if we need a new span due to style mismatch
    bool need_new_span = false;
    if (span->font != sveska->font) {
      //  printf("Font mismatch: span %p vs current %p\n", span->font, sveska->font);
        need_new_span = true;
    }
    if (fabs(span->font_size - sveska->font_size) > 0.1f) {
      //  printf("Font size mismatch: %.1f vs %.1f\n", span->font_size, sveska->font_size);
        need_new_span = true;
    }
    if (span->bold != sveska->bold || 
        span->italic != sveska->italic || 
        span->underline != sveska->underline) {
      //  printf("Style mismatch: B%dI%dU%d vs B%dI%dU%d\n",
       //        span->bold, span->italic, span->underline,
       //        sveska->bold, sveska->italic, sveska->underline);
        need_new_span = true;
    }
    
    if (need_new_span) {
     //   printf("Creating new span due to style mismatch\n");
        if (!create_new_span(sveska)) {
            printf("GRESKA: Neuspelo kreiranje novog spana\n");
            return;
        }
        // Update position after creating new span
        get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
        span = &sveska->document.spans[span_idx];
        //printf("New span: %zu, pos: %zu\n", span_idx, pos_in_span);
        
        // Validate new span
        if (!span || !span->text) {
            printf("GRESKA: Novi span je neispravan\n");
            return;
        }
    }

    // Ensure capacity
    if (span->length + 1 >= span->capacity) {
     //   printf("Reallocating span text (current capacity: %zu)\n", span->capacity);
        size_t new_capacity = span->capacity * 2;
        char *new_text = realloc(span->text, new_capacity);
        if (!new_text) {
            printf("GRESKA: Neuspela alokavija texta spana\n");
            return;
        }
        span->text = new_text;
        span->capacity = new_capacity;
    }

    // Record the operation before modifying the text
    char char_str[2] = {ch, '\0'};
    edit_operation_t op = {
        .type = OP_INSERT,
        .data.text.text = str_ndup(char_str, 1),
        .data.text.position = sveska->cursor_index,
        .data.text.length = 1,
        .affected_span = span_idx,
        .timestamp = get_timestamp()
    };
    
    // Save font info for undo
    if (sveska->font) {
        str_ncpy(op.data.text.font_path, MAX_FONT_PATH_LEN, 
                sveska->font->path, str_size(sveska->font->path));
    }
    op.data.text.font_size = sveska->font_size;
    
    record_edit(sveska, op);
   // printf("Recorded edit operation\n");

    // Make space for new character
    if (pos_in_span < span->length) {
        memmove(span->text + pos_in_span + 1, 
                span->text + pos_in_span, 
                span->length - pos_in_span);
    }
    
    // Insert character
    span->text[pos_in_span] = ch;
    span->length++;
    span->text[span->length] = '\0';  // Maintain null termination
    

    // Update document state
    sveska->cursor_index++;
  //  printf("Cursor index incremented to %u\n", sveska->cursor_index);
  sveska->document_modified = true;
    // Rebuild and render
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}


bool create_image_bitmap(sveska_t *sveska, text_span_t *span) {
    gfx_bitmap_params_t *params = &span->bitmap_params;
    gfx_bitmap_params_init(params);
    
    params->rect.p0.x = 0;
    params->rect.p0.y = 0;
    params->rect.p1.x = span->img_width;
    params->rect.p1.y = span->img_height;
    
    errno_t rc = gfx_bitmap_create(sveska->window_gc, params, NULL, &span->bitmap);
    if (rc != EOK) {
        printf("Failed to create bitmap: %d\n", rc);
        return false;
    }
    
    return true;
}

void insert_image_span(sveska_t *sveska, int width, int height) {
    if (!create_new_span(sveska)) return;

    text_span_t *span = get_current_span(sveska);
    span->is_image = true;
    span->img_width = width;
    span->img_height = height;
    span->length = 1; // Counts as one character
    
    // Create the bitmap
    if (!create_image_bitmap(sveska, span)) {
        // If bitmap creation fails, just use placeholder
        span->bitmap = NULL;
    }
    
    // Move cursor after image
    sveska->cursor_index++;
    sveska_text_render(sveska);
}

void insert_image_placeholder(sveska_t *sveska) {
    if (!sveska) return;
    
    // Delete any selected text first
    if (has_selection(sveska)) {
        delete_selected_text(sveska);
    }
    
    // Get current position information
    size_t current_span_idx, pos_in_span;
    get_span_and_pos(sveska, sveska->cursor_index, &current_span_idx, &pos_in_span);
    text_span_t *current_span = &sveska->document.spans[current_span_idx];
    
    // Split the current span at cursor if needed
    if (pos_in_span > 0 && pos_in_span < current_span->length) {
        if (!split_span_at(sveska, current_span_idx, pos_in_span)) {
            printf("Failed to split span for image insertion\n");
            return;
        }
        // After split, our target span is the new one
        current_span_idx++;
        pos_in_span = 0;
    }
    
    // Create a new span for the image at the current position
    if (sveska->document.count >= sveska->document.capacity) {
        // Expand capacity if needed
        size_t new_capacity = sveska->document.capacity * 2;
        text_span_t *new_spans = realloc(sveska->document.spans, new_capacity * sizeof(text_span_t));
        if (!new_spans) {
            printf("Failed to expand document capacity\n");
            return;
        }
        sveska->document.spans = new_spans;
        sveska->document.capacity = new_capacity;
    }
    
    // Make space for the new span
    memmove(&sveska->document.spans[current_span_idx + 1],
            &sveska->document.spans[current_span_idx],
            (sveska->document.count - current_span_idx) * sizeof(text_span_t));
    
    // Initialize the new image span
    text_span_t *image_span = &sveska->document.spans[current_span_idx];
    memset(image_span, 0, sizeof(text_span_t));
    image_span->is_image = true;
    image_span->img_width = 50;
    image_span->img_height = 50;
    image_span->length = 1;
    
    // Apply current styles to the image span
    image_span->bold = sveska->bold;
    image_span->italic = sveska->italic;
    image_span->underline = sveska->underline;
    image_span->font = sveska->font ? sveska_font_ref(sveska->font) : NULL;
    image_span->font_size = sveska->font_size;
    if (sveska->font) {
        str_ncpy(image_span->font_path, MAX_FONT_PATH_LEN, 
                sveska->font->path, str_size(sveska->font->path));
    }
    
    // Update document state
    sveska->document.count++;
    sveska->current_span = current_span_idx;
    
    // Move cursor after the image
    sveska->cursor_index++;
    
    // Record the operation
    edit_operation_t op = {
        .type = OP_INSERT,
        .data.text.text = str_ndup("[IMG]", 5),
        .data.text.position = sveska->cursor_index - 1, // Position before increment
        .data.text.length = 1,
        .affected_span = current_span_idx,
        .timestamp = get_timestamp()
    };
    record_edit(sveska, op);
    
    // Update UI
    sveska->document_modified = true;
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}



/** @}
 */
