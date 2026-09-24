
/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */

#include "sveska.h"
#include "sveska_keymap.h"
#include <clipboard.h>
#include <stdio.h>
#include <mem.h>
#include <math.h>

//static void insert_text_with_style_and_font(sveska_t *sveska, clipboard_data_t *clip_data, size_t start, size_t end);


//static void insert_text_with_style(sveska_t *sveska, clipboard_data_t *clip_data, size_t start, size_t end);



bool is_special_combo(const kbd_event_t *event) {
    if (!event) return false;
    return (event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) != 0;
}

void handle_copy(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("handle_copy: no selection\n");
        return;
    }

    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);
    size_t len = end - start;

    printf("Copying selection from %d to %d (%zu chars)\n", start, end, len);

    // Allocate memory for text and styles
    clipboard_data_t *clip_data = calloc(1, sizeof(clipboard_data_t));
    if (!clip_data) {
        printf("handle_copy: allocation failed for clip_data\n");
        return;
    }

    clip_data->text = malloc(len + 1);
    clip_data->bold = calloc(len, sizeof(bool));
    clip_data->italic = calloc(len, sizeof(bool));
    clip_data->underline = calloc(len, sizeof(bool));

    if (!clip_data->text || !clip_data->bold || !clip_data->italic || !clip_data->underline) {
        printf("handle_copy: style allocation failed\n");
        clipboard_free_data(clip_data);
        return;
    }

    // Get font info from first span in selection
    size_t first_span_idx, dummy_pos;
    get_span_and_pos(sveska, start, &first_span_idx, &dummy_pos);
    text_span_t *first_span = &sveska->document.spans[first_span_idx];

// After getting first_span
printf("First span font path: '%s'\n", first_span->font_path);
printf("First span font size: %.1f\n", first_span->font_size);
printf("Font pointer: %p\n", (void*)first_span->font);

clip_data->font_size = first_span->font ? first_span->font_size : sveska->font_size;
printf("Copying font SIZE: %s (%.1f)\n", clip_data->font_path, clip_data->font_size);
  
// Get font info - with more robust fallbacks
    if (first_span->font) {
        str_ncpy(clip_data->font_path, MAX_FONT_PATH_LEN, 
                first_span->font->path, str_size(first_span->font->path));
        clip_data->font_size = first_span->font->size;
    } else {
        // Fallback to current font if span has none
        str_ncpy(clip_data->font_path, MAX_FONT_PATH_LEN, 
                sveska->current_font_path, str_size(sveska->current_font_path));
        clip_data->font_size = sveska->font_size;
    }

    printf("Copying font: %s (%.1f)\n", clip_data->font_path, clip_data->font_size);

    // Copy text and styles
    for (int i = 0; i < (int)len; i++) {
        size_t global_pos = start + i;
        size_t span_idx, pos_in_span;
        get_span_and_pos(sveska, global_pos, &span_idx, &pos_in_span);

        text_span_t *span = &sveska->document.spans[span_idx];
        clip_data->text[i] = span->text[pos_in_span];

        bool b, it, ul;
        get_char_style(span, pos_in_span, &b, &it, &ul);
        
        clip_data->bold[i] = b;
        clip_data->italic[i] = it;
        clip_data->underline[i] = ul;
    }

    clip_data->text[len] = '\0';
    clip_data->length = len;

    // Serialize and store in clipboard
    char *serialized = serialize_styles(clip_data);
    if (serialized) {
        // Debug output before storing
        printf("Serialized data (%zu bytes): %s\n", str_size(serialized), serialized);
        
        // Store to system clipboard
        errno_t rc = clipboard_put_str(serialized);
        if (rc != EOK) {
            printf("Failed to store in clipboard: %d\n", rc);
        }
        
        free(serialized);
    }

    // DON'T free clip_data here - it's owned by the clipboard now
    // The system clipboard will manage its memory
}




void handle_paste(sveska_t *sveska) {
    if (!sveska) return;

    clipboard_data_t *clip_data = clipboard_get_rich_text(sveska);
    if (!clip_data || !clip_data->text || clip_data->length == 0) {
        printf("No valid data to paste\n");
        return;
    }

    // Delete any existing selection first
    if (has_selection(sveska)) {
        delete_selected_text(sveska);
    }

    // Save current style state
    bool old_bold = sveska->bold;
    bool old_italic = sveska->italic;
    bool old_underline = sveska->underline;
    char old_font_path[MAX_FONT_PATH_LEN];
    float old_font_size = sveska->font_size;
    str_ncpy(old_font_path, MAX_FONT_PATH_LEN, sveska->current_font_path, MAX_FONT_PATH_LEN);

    // Set font from clipboard data if available
    if (clip_data->font_path[0]) {
        printf("Attempting to set paste font: %s (%.1f)\n", 
               clip_data->font_path, clip_data->font_size);
        
        // Find font in pre-loaded list
        bool font_found = false;
        for (size_t i = 0; i < sveska->font_count; i++) {
            if (str_cmp(sveska->fonts[i].path, clip_data->font_path) == 0) {
                sveska_set_font(sveska, clip_data->font_path, clip_data->font_size);
                font_found = true;
                break;
            }
        }
        
        if (!font_found) {
            printf("Pasted font not found in pre-loaded list: %s\n", 
                   clip_data->font_path);
            // Fall back to current font but keep the size
            sveska->font_size = clip_data->font_size;
        }
    }

    // Record the operation before pasting
    edit_operation_t op = {
        .type = OP_INSERT,
        .data.text.text = str_ndup(clip_data->text, clip_data->length),
        .data.text.position = sveska->cursor_index,
        .data.text.length = clip_data->length,
        .affected_span = sveska->current_span,
        .timestamp = get_timestamp()
    };
    str_ncpy(op.data.text.font_path, MAX_FONT_PATH_LEN, 
            clip_data->font_path, str_size(clip_data->font_path));
    op.data.text.font_size = clip_data->font_size;

    // Insert text with proper styling
    for (size_t i = 0; i < clip_data->length; i++) {
        // Apply style for this character
        sveska->bold = clip_data->bold[i];
        sveska->italic = clip_data->italic[i];
        sveska->underline = clip_data->underline[i];

        // Create new span if style changed
        if (i == 0 || 
            clip_data->bold[i] != clip_data->bold[i-1] ||
            clip_data->italic[i] != clip_data->italic[i-1] ||
            clip_data->underline[i] != clip_data->underline[i-1]) {
            
            if (!create_new_span(sveska)) {
                printf("Failed to create new span for paste\n");
                break;
            }
        }

        // Insert the character
        char ch = clip_data->text[i];
        size_t span_idx, pos_in_span;
        get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
        text_span_t *span = &sveska->document.spans[span_idx];

        // Ensure capacity
        if (span->length >= span->capacity - 1) {
            size_t new_capacity = span->capacity * 2;
            char *new_text = realloc(span->text, new_capacity);
            if (!new_text) break;
            span->text = new_text;
            span->capacity = new_capacity;
        }

        // Insert character
        memmove(&span->text[pos_in_span + 1], &span->text[pos_in_span], 
               span->length - pos_in_span);
        span->text[pos_in_span] = ch;
        span->length++;
        span->text[span->length] = '\0';

        sveska->cursor_index++;
    }

    // Record the operation after pasting
    record_edit(sveska, op);

    // Restore original style state
    sveska->bold = old_bold;
    sveska->italic = old_italic;
    sveska->underline = old_underline;
    sveska_set_font(sveska, old_font_path, old_font_size);

    // Update display
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);

    clipboard_free_data(clip_data);
}


void handle_cut(sveska_t *sveska) {
    if (!sveska || !has_selection(sveska)) {
        printf("No selection to cut\n");
        return;
    }

    // Get selection range
    int start = MIN(sveska->selection.start_pos, sveska->selection.end_pos);
    int end = MAX(sveska->selection.start_pos, sveska->selection.end_pos);
    size_t len = end - start;

    // Create clipboard data structure
    clipboard_data_t *clip_data = malloc(sizeof(clipboard_data_t));
    if (!clip_data) {
        printf("Cut buffer allocation failed\n");
        return;
    }

    // Allocate arrays
    clip_data->text = malloc(len + 1);
    clip_data->bold = malloc(len);
    clip_data->italic = malloc(len);
    clip_data->underline = malloc(len);
    clip_data->length = len;

    if (!clip_data->text || !clip_data->bold || !clip_data->italic || !clip_data->underline) {
        printf("Cut style buffers allocation failed\n");
        clipboard_free_data(clip_data);
        return;
    }

    // Copy text and styles
    for (int i = 0; i < (int)len; i++) {
        size_t global_pos = start + i;
        size_t span_idx, span_pos;
        get_span_and_pos(sveska, global_pos, &span_idx, &span_pos);
        
        text_span_t *span = &sveska->document.spans[span_idx];
        clip_data->text[i] = span->text[span_pos];
        clip_data->bold[i] = span->bold;
        clip_data->italic[i] = span->italic;
        clip_data->underline[i] = span->underline;
    }
    clip_data->text[len] = '\0';

    // Get font info from first span
    size_t first_span_idx, dummy;
    get_span_and_pos(sveska, start, &first_span_idx, &dummy);
    text_span_t *first_span = &sveska->document.spans[first_span_idx];
    
    if (first_span->font) {
        str_ncpy(clip_data->font_path, MAX_FONT_PATH_LEN, 
                sveska->current_font_path, str_size(sveska->current_font_path));
        clip_data->font_size = sveska->font_size;
    } else {
        str_ncpy(clip_data->font_path, MAX_FONT_PATH_LEN, 
                "/data/font/arial.ttf", str_size("/data/font/arial.ttf"));
        clip_data->font_size = 12.0f;
    }

    // Put to clipboard
    errno_t rc = clipboard_put_rich_text(sveska, clip_data);
    if (rc != EOK) {
        printf("Clipboard put failed: %d\n", rc);
        clipboard_free_data(clip_data);
        return;
    }

    // Delete selected text
    delete_selected_text(sveska);
    clipboard_free_data(clip_data);
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}



void handle_special_combo(sveska_t *sveska, const kbd_event_t *event) {
    if (!sveska || !event) return;

    if ((event->mods & KM_CTRL) && !(event->mods & KM_ALT)) {
        switch (event->key) {
            
            case KC_X:  // Ctrl+X
            handle_cut(sveska);
            break;

            case KC_C:  // Ctrl+C
                if (has_selection(sveska)) {
                    handle_copy(sveska);
                    return;  // Handled - don't pass to default handler
                }
                break;
                
            case KC_V:  // Ctrl+V
                handle_paste(sveska);
                break;
                
            case KC_S:
                printf("Ctrl+S pressed - Save command\n");
                break;
                
            case KC_O:
                printf("Ctrl+O pressed - Open command\n");
                break;
                
            case KC_N:
                printf("Ctrl+N pressed - New document\n");
                break;
                
            case KC_B:
                sveska_toggle_bold(sveska);
                break;
                
            case KC_I:
                sveska->italic = !sveska->italic;
                rebuild_display_text(sveska);
                break;
                
            case KC_U:
                sveska->underline = !sveska->underline;
                rebuild_display_text(sveska);
                break;
                
                case KC_Z:  // Ctrl+Z - Undo
                perform_undo(sveska);
                rebuild_display_text(sveska);
                sveska_text_render(sveska);
                break;
                
            case KC_Y:  // Ctrl+Y - Redo
                perform_redo(sveska);
                rebuild_display_text(sveska);
                sveska_text_render(sveska);
                break;
                
            case KC_BACKSPACE:
                if (has_selection(sveska)) {
                delete_selected_text(sveska);
                } else {
                handle_backspace(sveska);
                }
                break;

            case KC_CAPS_LOCK:
                sveska->caps_lock_on = !sveska->caps_lock_on;
                // Update UI indicator if needed
                return;
                
            default:
                printf("Unhandled Ctrl+key: 0x%X\n", event->key);
                break;
        }
        return;
    }

    // Handle Alt combinations
    if ((event->mods & KM_ALT) && !(event->mods & KM_CTRL)) {
        switch (event->key) {
            case KC_1 ... KC_9:
                printf("Alt+%d stisnut\n", event->key - KC_1 + 1);
                break;
            default:
                printf("Nepodrzan Alt+key: 0x%X\n", event->key);
        }
        return;
    }

    // Handle Shift combinations
    if ((event->mods & KM_SHIFT) && !(event->mods & (KM_CTRL | KM_ALT))) {
        char ch = translate_key_to_char(event);
        if (ch) {
            handle_keypress(sveska, event);
        }
    }

        // Pass to default handler if not handled
       return;
}

void handle_keyboard_event(sveska_t *sveska, const kbd_event_t *event) {
    if (!sveska || !event || event->type != KEY_PRESS) return;

    if (is_special_combo(event)) {
        handle_special_combo(sveska, event);
    } else {
        handle_regular_key(sveska, event);
    }
}
/******************************************* */

void handle_regular_key(sveska_t *sveska, const kbd_event_t *event) {
    if (!sveska || !event) return;

    switch (event->key) {
        case KC_ENTER:
        case KC_NENTER:
            handle_newline(sveska);
            return;

        case KC_BACKSPACE:
            if (has_selection(sveska)) {
                delete_selected_text(sveska);
            } else {
                handle_backspace(sveska);
            }
            break;
            
        case KC_LEFT:
            move_cursor_left(sveska);
            break;
            
        case KC_RIGHT:
            move_cursor_right(sveska);
            break;
            
        case KC_UP:
            move_cursor_up(sveska);
            break;
            
        case KC_DOWN:
            move_cursor_down(sveska);
            break;
            
        case KC_HOME:
            move_cursor_home(sveska, event->mods & KM_SHIFT);
            break;
            
        case KC_END:
            move_cursor_end(sveska, event->mods & KM_SHIFT);
            break;

        case KC_DELETE:
            if (has_selection(sveska)) {
                delete_selected_text(sveska);
            } else if (sveska->cursor_index < get_document_length(sveska)) {
                // Delete character after cursor
                sveska->cursor_index++;
                handle_backspace(sveska);
            }
            break;
        case KC_ESCAPE:
            // Clear selection
            clear_selection(sveska);
            break;
        /*
        case KC_PAGE_UP:
            // Scroll up one page (implement this)
            scroll_page(sveska, -1);
            break;
        case KC_PAGE_DOWN:
            // Scroll down one page (implement this)
            scroll_page(sveska, 1);
            break;
       */
        case KC_INSERT:
            sveska->overwrite_mode = !sveska->overwrite_mode;
            break;
        
        default: {
            char ch = translate_key_to_char(event);
            if (ch) {
                insert_char_with_current_style(sveska, ch);
            }
            break;
        }
    }
}

void perform_undo(sveska_t *sveska) {
    if (!sveska) {
        printf("perform_undo: neispravna instanca sveske\n");
        return;
    }

    if (sveska->history_pos == 0) {
        printf("Nista za undo\n");
        return;
    }

    // Document validation
    if (sveska->document.count == 0 || !sveska->document.spans) {
        printf("Dokument neispravan, reiniciram...\n");
        if (!initialize_document(sveska)) {
            printf("Neuspela reinicilizacija praznog dokumenta\n");
            return;
        }
    }

    // Move history position back
    sveska->history_pos--;
    edit_operation_t *op = &sveska->edit_history[sveska->history_pos];

    // Operation validation
    if (op->type < OP_INSERT || op->type > OP_FORMAT) {
        printf("Neispravan tip operacije u istoriji: %d\n", op->type);
        sveska->history_pos++; // Revert position change
        return;
    }

    // Position validation for text operations
    if (op->type == OP_INSERT || op->type == OP_DELETE) {
        size_t doc_length = get_document_length(sveska);
        if (op->data.text.position > doc_length) {
            printf("Podesavam neispravnu poziciju %zu to %zu\n",
                  op->data.text.position, doc_length);
            op->data.text.position = doc_length;
        }

        if (op->data.text.text == NULL && op->data.text.length > 0) {
            printf("Pozor: Operacija ima duzimu %zu ali NULL texta\n",
                  op->data.text.length);
        }
    }

    // Span validation
    if (op->affected_span >= sveska->document.count) {
        printf("Podesavam pogresno afektovan span %zu to %zu\n",
              op->affected_span, sveska->document.count - 1);
        op->affected_span = sveska->document.count > 0 ? sveska->document.count - 1 : 0;
    }

    // Apply the operation
    apply_operation(sveska, op, true);

    // Force UI update
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}

void perform_redo(sveska_t *sveska) {
    if (!sveska) {
        printf("perform_redo: neispravna instanca sveske\n");
        return;
    }

    if (sveska->history_pos >= sveska->history_size) {
        printf("Nista za redo\n");
        return;
    }

    // Document validation
    if (sveska->document.count == 0 || !sveska->document.spans) {
        printf("Dokument neispravan, reinicijalizacija...\n");
        if (!initialize_document(sveska)) {
            printf("Neuspela reinicijalizacija praznog dokumenta\n");
            return;
        }
    }

    edit_operation_t *op = &sveska->edit_history[sveska->history_pos];

    // Operation validation
    if (op->type < OP_INSERT || op->type > OP_FORMAT) {
        printf("Neisptravan tip operacija u istoriji: %d\n", op->type);
        return;
    }

    // Position validation for text operations
    if (op->type == OP_INSERT || op->type == OP_DELETE) {
        size_t doc_length = get_document_length(sveska);
        if (op->data.text.position > doc_length) {
            printf("Podesavam neispravnu poziciju %zu u %zu\n",
                  op->data.text.position, doc_length);
            op->data.text.position = doc_length;
        }

        if (op->data.text.text == NULL && op->data.text.length > 0) {
            printf("Pozor: Operacija ima duzinu %zu ali NULL texta\n",
                  op->data.text.length);
        }
    }

    // Span validation
    if (op->affected_span >= sveska->document.count) {
        printf("podesavam pogresno afektovan span %zu u %zu\n",
              op->affected_span, sveska->document.count - 1);
        op->affected_span = sveska->document.count > 0 ? sveska->document.count - 1 : 0;
    }

    // Apply the operation
    apply_operation(sveska, op, false);
    sveska->history_pos++;

    // Force UI update
    rebuild_display_text(sveska);
    update_cursor_position(sveska);
    sveska_text_render(sveska);
}

/**
 * Inserts text with explicit styling (for paste operations)
 */
 void insert_text_with_style(sveska_t *sveska, const char *text, size_t length,
    const bool *bold, const bool *italic, const bool *underline,
    const char *font_path, float font_size) {
if (!sveska || !text || length == 0) return;

// Save current style state
bool old_bold = sveska->bold;
bool old_italic = sveska->italic;
bool old_underline = sveska->underline;
char old_font_path[MAX_FONT_PATH_LEN];
float old_font_size = sveska->font_size;
str_ncpy(old_font_path, MAX_FONT_PATH_LEN, sveska->current_font_path, MAX_FONT_PATH_LEN);

// Set initial font if provided
if (font_path && font_path[0] != '\0') {
sveska_set_font(sveska, font_path, font_size);
}

for (size_t i = 0; i < length; i++) {
// Check if style changed from previous character
bool style_changed = (i == 0) ||
     (bold && bold[i] != sveska->bold) ||
     (italic && italic[i] != sveska->italic) ||
     (underline && underline[i] != sveska->underline);

if (style_changed) {
// Update current style
if (bold) sveska->bold = bold[i];
if (italic) sveska->italic = italic[i];
if (underline) sveska->underline = underline[i];

// Create new span if needed
size_t span_idx, pos_in_span;
get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
text_span_t *span = &sveska->document.spans[span_idx];

if (needs_new_span(sveska, span)) {
if (!create_new_span(sveska)) {
printf("Neuspelo kreiranje novog spana za stilizovan text\n");
break;
}
}
}

// Insert the character
char ch = text[i];
size_t span_idx, pos_in_span;
get_span_and_pos(sveska, sveska->cursor_index, &span_idx, &pos_in_span);
text_span_t *span = &sveska->document.spans[span_idx];

// Ensure capacity
if (span->length >= span->capacity - 1) {
size_t new_capacity = span->capacity * 2;
char *new_text = realloc(span->text, new_capacity);
if (!new_text) break;
span->text = new_text;
span->capacity = new_capacity;
}

// Insert character
memmove(&span->text[pos_in_span + 1], &span->text[pos_in_span], 
span->length - pos_in_span);
span->text[pos_in_span] = ch;
span->length++;
span->text[span->length] = '\0';

sveska->cursor_index++;
}

// Restore original style state
sveska->bold = old_bold;
sveska->italic = old_italic;
sveska->underline = old_underline;
sveska_set_font(sveska, old_font_path, old_font_size);
}



/** @}
 */
