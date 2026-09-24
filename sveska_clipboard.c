
/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */

#include "sveska.h"
#include <clipboard.h>
#include <mem.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>


 #define CLIPBOARD_VERSION 3  // Updated version for new format

/* New robust serialization format:
 * VERSION|FONT_PATH|FONT_SIZE|TEXT_LENGTH|TEXT_DATA|STYLE_DATA
 * Where:
 * - VERSION: integer
 * - FONT_PATH: string
 * - FONT_SIZE: float as string
 * - TEXT_LENGTH: integer
 * - TEXT_DATA: raw text
 * - STYLE_DATA: packed B/I/U flags (one byte per char: 'B' or 'b', etc)
 */




 char *serialize_styles(clipboard_data_t *data) {
    if (!data) return NULL;
    
    
    if (!data->font_path[0]) {
        printf("POZOR: Nema putanje fonta u deserijalizaciji!\n");
        str_ncpy(data->font_path, MAX_FONT_PATH_LEN, "default_font.ttf", 16);
    }


    // Calculate required buffer size
    size_t needed = 256 + data->length * 4; // Extra space for metadata
    char *buffer = malloc(needed);
    if (!buffer) return NULL;

    // Format: VERSION|FONT_PATH|FONT_SIZE|TEXT_LENGTH|TEXT|BOLD|ITALIC|UNDERLINE
    int offset = snprintf(buffer, needed, "%d|%s|%.1f|%zu|",
                        CLIPBOARD_VERSION,
                        data->font_path,
                        data->font_size,
                        data->length);

    // Copy text
    memcpy(buffer + offset, data->text, data->length);
    offset += data->length;
    buffer[offset++] = '|';

    // Copy style flags
    for (size_t i = 0; i < data->length; i++) {
        buffer[offset++] = data->bold[i] ? '1' : '0';
    }
    buffer[offset++] = '|';

    for (size_t i = 0; i < data->length; i++) {
        buffer[offset++] = data->italic[i] ? '1' : '0';
    }
    buffer[offset++] = '|';

    for (size_t i = 0; i < data->length; i++) {
        buffer[offset++] = data->underline[i] ? '1' : '0';
    }
    buffer[offset] = '\0';

    return buffer;
}


clipboard_data_t *deserialize_styles(const char *buffer) {
    if (!buffer) return NULL;

    clipboard_data_t *data = calloc(1, sizeof(clipboard_data_t));
    if (!data) return NULL;

    // Simple format: VERSION|FONT_PATH|FONT_SIZE|TEXT_LENGTH|TEXT|BOLD|ITALIC|UNDERLINE
    const char *ptr = buffer;
    const char *end = str_chr(ptr, '|');
    if (!end) goto fail;
    
    // Parse version
    int version = atoi(ptr);
    if (version != CLIPBOARD_VERSION) {
        printf("Nepodrzana klipboard verzija: %d\n", version);
        goto fail;
    }
    ptr = end + 1;

    // Parse font path
    end = str_chr(ptr, '|');
    if (!end) goto fail;
    str_ncpy(data->font_path, MAX_FONT_PATH_LEN, ptr, end - ptr);
    ptr = end + 1;

    // Parse font size
    end = str_chr(ptr, '|');
    if (!end) goto fail;
    data->font_size = atof(ptr);
    ptr = end + 1;

    // Parse text length
    end = str_chr(ptr, '|');
    if (!end) goto fail;
    data->length = atoi(ptr);
    ptr = end + 1;

    // Validate length
    if (data->length <= 0 || data->length > 1000000) { // Sanity check
        printf("Neispravna duzina texta: %zu\n", data->length);
        goto fail;
    }

    // Allocate memory
    data->text = malloc(data->length + 1);
    data->bold = malloc(data->length);
    data->italic = malloc(data->length);
    data->underline = malloc(data->length);
    if (!data->text || !data->bold || !data->italic || !data->underline) goto fail;

    // Copy text
    if ((size_t)(end - ptr) < data->length) {
        printf("Duzina texta nije OK\n");
        goto fail;
    }
    memcpy(data->text, ptr, data->length);
    data->text[data->length] = '\0';
    ptr += data->length;
    if (*ptr != '|') goto fail;
    ptr++;

    // Parse bold flags
    for (size_t i = 0; i < data->length; i++) {
        if (*ptr == '|' || *ptr == '\0') goto fail;
        data->bold[i] = (*ptr++ == '1');
    }
    if (*ptr != '|') goto fail;
    ptr++;

    // Parse italic flags
    for (size_t i = 0; i < data->length; i++) {
        if (*ptr == '|' || *ptr == '\0') goto fail;
        data->italic[i] = (*ptr++ == '1');
    }
    if (*ptr != '|') goto fail;
    ptr++;

    // Parse underline flags
    for (size_t i = 0; i < data->length; i++) {
        if (*ptr == '|' || *ptr == '\0') break;
        data->underline[i] = (*ptr++ == '1');
    }

    return data;

fail:
    printf("Neuspelo parsiranje podataka klipboarda\n");
    clipboard_free_data(data);
    return NULL;
}




errno_t clipboard_put_rich_text(sveska_t *sveska, clipboard_data_t *data) {
    if (!sveska || !data) {
        printf("clipboard_put_rich_text: Neispravan argument\n");
        return EINVAL;
    }

    // Serialize the clipboard data to a string
    char *serialized = serialize_styles(data);
    if (!serialized) {
        printf("clipboard_put_rich_text: neuspela serijalizacija\n");
        return ENOMEM;
    }

    // Debug output
   // printf("Serialized (%zu bytes):\n%s\n", str_size(serialized), serialized);

    // Save the serialized string to the system clipboard
    errno_t rc = clipboard_put_str(serialized);

    // Optional: write serialized output to a file for verification
    FILE *f = fopen("serialized_put_rich.txt", "w");
    if (f) {
        fputs(serialized, f);
        fclose(f);
    } else {
        printf("clipboard_put_rich_text: neuspelo otvaranje serialized.txt zxa upis\n");
    }

    free(serialized);
    return rc;
}





clipboard_data_t *clipboard_get_rich_text(sveska_t *sveska) {
    if (!sveska) {
        printf("clipboard_get_rich_text: neispravna sveska\n");
        return NULL;
    }

    char *buffer = NULL;
    errno_t rc = clipboard_get_str(&buffer);
    if (rc != EOK || !buffer) {
        printf("clipboard_get_rich_text: get_str neuspelo (%d) ili je klipboard prazan\n", rc);
        return NULL;
    }

   // printf("Retrieved from clipboard (%zu bytes):\n%s\n", str_size(buffer), buffer);

    // Save raw string from clipboard BEFORE freeing
    FILE *f_clip_raw = fopen("clipboard_raw.txt", "w");
    if (f_clip_raw) {
        fputs(buffer, f_clip_raw);
        fclose(f_clip_raw);
    } else {
        printf("Ne mogu da otvorim clipboard_raw.txt za upis\n");
    }

    clipboard_data_t *data = deserialize_styles(buffer);
    free(buffer); // <-- now it's safe

    if (data) {
        FILE *f_deserialized = fopen("deserialized.txt", "w");
        if (f_deserialized) {
            fprintf(f_deserialized, "Font path: %s\n", data->font_path);
            fprintf(f_deserialized, "Font size: %.1f\n", data->font_size);
            fprintf(f_deserialized, "Text: %s\n", data->text);

            fprintf(f_deserialized, "Bold: ");
            for (size_t i = 0; i < data->length; ++i)
                fputc(data->bold[i] ? '1' : '0', f_deserialized);
            fputc('\n', f_deserialized);

            fprintf(f_deserialized, "Italic: ");
            for (size_t i = 0; i < data->length; ++i)
                fputc(data->italic[i] ? '1' : '0', f_deserialized);
            fputc('\n', f_deserialized);

            fprintf(f_deserialized, "Underline: ");
            for (size_t i = 0; i < data->length; ++i)
                fputc(data->underline[i] ? '1' : '0', f_deserialized);
            fputc('\n', f_deserialized);

            fclose(f_deserialized);
        } else {
            printf("Ne mogu da otvaorim deserialized.txt za upis\n");
        }
    } else {
        printf("clipboard_get_rich_text: neuspela deserijalizacija\n");
    }

    return data;
}


void clipboard_free_data(clipboard_data_t *data) {
    if (!data) return;

    if (data->text) free(data->text);
    if (data->bold) free(data->bold);
    if (data->italic) free(data->italic);
    if (data->underline) free(data->underline);
    free(data);
}


void free_span(text_span_t *span) {
    if (span->text) free(span->text);
    if (span->bold_array) free(span->bold_array);
    if (span->italic_array) free(span->italic_array);
    if (span->underline_array) free(span->underline_array);
}

void get_char_style(text_span_t *span, size_t pos_in_span,
    bool *bold, bool *italic, bool *underline) {
    if (!span || pos_in_span >= span->length) {
        *bold = false;
        *italic = false;
        *underline = false;
        return;
    }

    *bold = (span->bold_array) ? span->bold_array[pos_in_span] : span->bold;
    *italic = (span->italic_array) ? span->italic_array[pos_in_span] : span->italic;
    *underline = (span->underline_array) ? span->underline_array[pos_in_span] : span->underline;
}


void ensure_style_arrays(text_span_t *span) {
    if (!span->bold_array) {
        span->bold_array = calloc(span->capacity, sizeof(bool));
        for (size_t i = 0; i < span->length; i++) {
            span->bold_array[i] = span->bold;
        }
    }
    if (!span->italic_array) {
        span->italic_array = calloc(span->capacity, sizeof(bool));
        for (size_t i = 0; i < span->length; i++) {
            span->italic_array[i] = span->italic;
        }
    }
    if (!span->underline_array) {
        span->underline_array = calloc(span->capacity, sizeof(bool));
        for (size_t i = 0; i < span->length; i++) {
            span->underline_array[i] = span->underline;
        }
    }
}



/** @}
 */