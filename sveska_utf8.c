

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */



#include "sveska.h"
#include <stdbool.h>

// Returns the number of bytes in a UTF-8 character

size_t utf8_char_length(const char *str) {
    unsigned char c = (unsigned char)str[0];
    if (c < 0x80) return 1;        // ASCII
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;  // Invalid UTF-8, treat as single byte
}

// Extracts a UTF-8 character
utf8_char_t get_utf8_char(const char *str, size_t pos) {
    utf8_char_t ch = { .bytes = {0}, .length = 0 };
    size_t len = utf8_char_length(str + pos);
    
    if (len > 0 && len <= UTF8_MAX_BYTES) {
        memcpy(ch.bytes, str + pos, len);
        ch.length = len;
    } else {
        // Fallback for invalid UTF-8
        ch.bytes[0] = str[pos];
        ch.length = 1;
    }
    return ch;
}

// Counts UTF-8 characters (not bytes)
int utf8_strlen(const char *str) {
    int count = 0;
    while (*str) {
        str += utf8_char_length(str);
        count++;
    }
    return count;
}

// Initialize UTF-8 functions in your sveska_t struct
void sveska_init_utf8(sveska_t *sveska) {
    sveska->utf8_char_length = utf8_char_length;
    sveska->get_utf8_char = get_utf8_char;
    sveska->utf8_strlen = utf8_strlen;
}



/** @}
 */
