

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */


#include "sveska.h"
#include "sveska_keymap.h"
#include <io/keycode.h>


static char translate_key_to_char_us(const kbd_event_t *event);
static char translate_key_to_char_serbian_latin(const kbd_event_t *event);

utf8_char_t translate_key_to_char_serbian_cyrillic(const kbd_event_t *event);
void update_layout_indicator() ;
void load_keyboard_layout() ;
void save_keyboard_layout(keyboard_layout_t layout) ;

char translate_key_to_char(const kbd_event_t *event) {
    if (!sveska) return 0; // Safety check
    
    // Handle control keys the same for all layouts
    switch (event->key) {
        case KC_ENTER:    return '\n';
        case KC_NENTER:   return '\n';
        case KC_TAB:      return '\t';
        case KC_SPACE:    return ' ';
        case KC_BACKSPACE:return '\b';
        case KC_CAPS_LOCK:     // Caps Lock key
            sveska->caps_lock_on = !sveska->caps_lock_on;
            return 0; // No character to insert, just toggle state
        default: break;
    }

    char ch = 0;
    
    // Always use Serbian Latin layout if that's what's set
    if (sveska->current_layout == KEYBOARD_LAYOUT_SERBIAN_LATIN) {
        ch = translate_key_to_char_serbian_latin(event);
    } else {
        // Fall back to US layout for any other case
        ch = translate_key_to_char_us(event);
    }

    // Apply Caps Lock transformation for letters
    if (ch && sveska->caps_lock_on) {
        if (ch >= 'a' && ch <= 'z') {
            // Uppercase when Caps Lock is on
            ch = ch - 'a' + 'A';
        } else if (ch >= 'A' && ch <= 'Z' && (event->mods & KM_SHIFT)) {
            // Lowercase when Shift is pressed with Caps Lock on
            ch = ch - 'A' + 'a';
        }
    }
    
    return ch;
}

static char translate_key_to_char_us(const kbd_event_t *event) {
    switch (event->key) {
        /* Letters */
        case KC_Q: return (event->mods & KM_SHIFT) ? 'Q' : 'q';
        case KC_W: return (event->mods & KM_SHIFT) ? 'W' : 'w';
        case KC_E: return (event->mods & KM_SHIFT) ? 'E' : 'e';
        case KC_R: return (event->mods & KM_SHIFT) ? 'R' : 'r';
        case KC_T: return (event->mods & KM_SHIFT) ? 'T' : 't';
        case KC_Y: return (event->mods & KM_SHIFT) ? 'Y' : 'y';
        case KC_U: return (event->mods & KM_SHIFT) ? 'U' : 'u';
        case KC_I: return (event->mods & KM_SHIFT) ? 'I' : 'i';
        case KC_O: return (event->mods & KM_SHIFT) ? 'O' : 'o';
        case KC_P: return (event->mods & KM_SHIFT) ? 'P' : 'p';
        
        case KC_A: return (event->mods & KM_SHIFT) ? 'A' : 'a';
        case KC_S: return (event->mods & KM_SHIFT) ? 'S' : 's';
        case KC_D: return (event->mods & KM_SHIFT) ? 'D' : 'd';
        case KC_F: return (event->mods & KM_SHIFT) ? 'F' : 'f';
        case KC_G: return (event->mods & KM_SHIFT) ? 'G' : 'g';
        case KC_H: return (event->mods & KM_SHIFT) ? 'H' : 'h';
        case KC_J: return (event->mods & KM_SHIFT) ? 'J' : 'j';
        case KC_K: return (event->mods & KM_SHIFT) ? 'K' : 'k';
        case KC_L: return (event->mods & KM_SHIFT) ? 'L' : 'l';
        
        case KC_Z: return (event->mods & KM_SHIFT) ? 'Z' : 'z';
        case KC_X: return (event->mods & KM_SHIFT) ? 'X' : 'x';
        case KC_C: return (event->mods & KM_SHIFT) ? 'C' : 'c';
        case KC_V: return (event->mods & KM_SHIFT) ? 'V' : 'v';
        case KC_B: return (event->mods & KM_SHIFT) ? 'B' : 'b';
        case KC_N: return (event->mods & KM_SHIFT) ? 'N' : 'n';
        case KC_M: return (event->mods & KM_SHIFT) ? 'M' : 'm';

        /* Numbers */
        case KC_1: return (event->mods & KM_SHIFT) ? '!' : '1';
        case KC_2: return (event->mods & KM_SHIFT) ? '@' : '2';
        case KC_3: return (event->mods & KM_SHIFT) ? '#' : '3';
        case KC_4: return (event->mods & KM_SHIFT) ? '$' : '4';
        case KC_5: return (event->mods & KM_SHIFT) ? '%' : '5';
        case KC_6: return (event->mods & KM_SHIFT) ? '^' : '6';
        case KC_7: return (event->mods & KM_SHIFT) ? '&' : '7';
        case KC_8: return (event->mods & KM_SHIFT) ? '*' : '8';
        case KC_9: return (event->mods & KM_SHIFT) ? '(' : '9';
        case KC_0: return (event->mods & KM_SHIFT) ? ')' : '0';

        /* Special chars */
        case KC_BACKTICK:  return (event->mods & KM_SHIFT) ? '~' : '`';
        case KC_MINUS:     return (event->mods & KM_SHIFT) ? '_' : '-';
        case KC_EQUALS:    return (event->mods & KM_SHIFT) ? '+' : '=';
        case KC_LBRACKET:  return (event->mods & KM_SHIFT) ? '{' : '[';
        case KC_RBRACKET:  return (event->mods & KM_SHIFT) ? '}' : ']';
        case KC_SEMICOLON: return (event->mods & KM_SHIFT) ? ':' : ';';
        case KC_QUOTE:     return (event->mods & KM_SHIFT) ? '"' : '\'';
        case KC_BACKSLASH: return (event->mods & KM_SHIFT) ? '|' : '\\';
        case KC_COMMA:     return (event->mods & KM_SHIFT) ? '<' : ',';
        case KC_PERIOD:    return (event->mods & KM_SHIFT) ? '>' : '.';
        case KC_NPERIOD:   return '.';
        case KC_SLASH:     return (event->mods & KM_SHIFT) ? '?' : '/';



        case KC_F1 ... KC_F12: return 0;

        case KC_PAGE_UP:    
        case KC_PAGE_DOWN:    
        case KC_INSERT:     
        case KC_DELETE:     
        case KC_ESCAPE:     return 0;

        default: return 0;
    }
}



static char translate_key_to_char_serbian_latin(const kbd_event_t *event) {
    // Handle AltGr combinations first (for special Serbian characters)
    if (event->mods & KM_ALT) {
        switch (event->key) {
            case KC_S: return (event->mods & KM_SHIFT) ? 0x8A : 0x9A; // Š/š (0xC5/0xE5)
            case KC_D: return (event->mods & KM_SHIFT) ? 0x8F : 0x9F; // Đ/đ (0xD0/0xF0)
            case KC_C: return (event->mods & KM_SHIFT) ? 0x8C : 0x9C; // Č/č (0xC4/0xE4)
            case KC_Z: return (event->mods & KM_SHIFT) ? 0x8E : 0x9E; // Ž/ž (0xC5/0xE5)
            case KC_LBRACKET: return (event->mods & KM_SHIFT) ? 0x86 : 0x87; // Ć/ć (0xC4/0xE4)
            default: break;
        }
    }

    // Handle special Serbian characters on their standard positions
    switch (event->key) {
        case KC_BACKSLASH: return (event->mods & KM_SHIFT) ? 0x9E : 0x9E; // ž (always lowercase)
        case KC_RBRACKET:  return (event->mods & KM_SHIFT) ? 0x9F : 0x9F; // đ (always lowercase)
        case KC_LBRACKET:  return (event->mods & KM_SHIFT) ? 0x9A : 0x9A; // š (always lowercase)
        case KC_QUOTE:     return (event->mods & KM_SHIFT) ? 0x87 : 0x87; // ć (always lowercase)
        case KC_SEMICOLON: return (event->mods & KM_SHIFT) ? 0x9C : 0x9C; // č (always lowercase)
        default: break;
    }

    // Standard Serbian Latin QWERTZ layout
    switch (event->key) {
        /* Letters */
        case KC_Q: return (event->mods & KM_SHIFT) ? 'Q' : 'q';
        case KC_W: return (event->mods & KM_SHIFT) ? 'W' : 'w';
        case KC_E: return (event->mods & KM_SHIFT) ? 'E' : 'e';
        case KC_R: return (event->mods & KM_SHIFT) ? 'R' : 'r';
        case KC_T: return (event->mods & KM_SHIFT) ? 'T' : 't';
        case KC_Y: return (event->mods & KM_SHIFT) ? 'Z' : 'z'; // Y and Z swapped
        case KC_U: return (event->mods & KM_SHIFT) ? 'U' : 'u';
        case KC_I: return (event->mods & KM_SHIFT) ? 'I' : 'i';
        case KC_O: return (event->mods & KM_SHIFT) ? 'O' : 'o';
        case KC_P: return (event->mods & KM_SHIFT) ? 'P' : 'p';
        
        case KC_A: return (event->mods & KM_SHIFT) ? 'A' : 'a';
        case KC_S: return (event->mods & KM_SHIFT) ? 'S' : 's';
        case KC_D: return (event->mods & KM_SHIFT) ? 'D' : 'd';
        case KC_F: return (event->mods & KM_SHIFT) ? 'F' : 'f';
        case KC_G: return (event->mods & KM_SHIFT) ? 'G' : 'g';
        case KC_H: return (event->mods & KM_SHIFT) ? 'H' : 'h';
        case KC_J: return (event->mods & KM_SHIFT) ? 'J' : 'j';
        case KC_K: return (event->mods & KM_SHIFT) ? 'K' : 'k';
        case KC_L: return (event->mods & KM_SHIFT) ? 'L' : 'l';
        
        case KC_Z: return (event->mods & KM_SHIFT) ? 'Y' : 'y'; // Y and Z swapped
        case KC_X: return (event->mods & KM_SHIFT) ? 'X' : 'x';
        case KC_C: return (event->mods & KM_SHIFT) ? 'C' : 'c';
        case KC_V: return (event->mods & KM_SHIFT) ? 'V' : 'v';
        case KC_B: return (event->mods & KM_SHIFT) ? 'B' : 'b';
        case KC_N: return (event->mods & KM_SHIFT) ? 'N' : 'n';
        case KC_M: return (event->mods & KM_SHIFT) ? 'M' : 'm';

        /* Numbers and special characters */
        case KC_1: return (event->mods & KM_SHIFT) ? '!' : '1';
        case KC_2: return (event->mods & KM_SHIFT) ? '"' : '2';
        case KC_3: return (event->mods & KM_SHIFT) ? '#' : '3';
        case KC_4: return (event->mods & KM_SHIFT) ? '$' : '4';
        case KC_5: return (event->mods & KM_SHIFT) ? '%' : '5';
        case KC_6: return (event->mods & KM_SHIFT) ? '&' : '6';
        case KC_7: return (event->mods & KM_SHIFT) ? '/' : '7';
        case KC_8: return (event->mods & KM_SHIFT) ? '(' : '8';
        case KC_9: return (event->mods & KM_SHIFT) ? ')' : '9';
        case KC_0: return (event->mods & KM_SHIFT) ? '=' : '0';
        
        case KC_BACKTICK:  return (event->mods & KM_SHIFT) ? '~' : '`';
        case KC_MINUS:     return (event->mods & KM_SHIFT) ? '_' : '-';
        case KC_EQUALS:    return (event->mods & KM_SHIFT) ? '+' : '=';
        case KC_COMMA:     return (event->mods & KM_SHIFT) ? '<' : ',';
        case KC_PERIOD:    return (event->mods & KM_SHIFT) ? '>' : '.';

        default: return 0;
    }
}

bool is_serbian_key(int key) {
    // List of keys that can produce Serbian characters
    return (key == KC_S || key == KC_D || key == KC_C || 
            key == KC_Z || key == KC_L || key == KC_N);
}

/*
void save_keyboard_layout(keyboard_layout_t layout) {
    // Save to config file or registry
    const char *layout_str;
    switch (layout) {
        case KEYBOARD_LAYOUT_SERBIAN_LATIN: layout_str = "sr_latin"; break;
        case KEYBOARD_LAYOUT_SERBIAN_CYRILLIC: layout_str = "sr_cyrillic"; break;
        default: layout_str = "us"; break;
    }
    config_set_string("keyboard_layout", layout_str);
}

void load_keyboard_layout() {
    const char *layout = config_get_string("keyboard_layout", "us");
    if (strcmp(layout, "sr_latin") == 0) {
        sveska->current_layout = KEYBOARD_LAYOUT_SERBIAN_LATIN;
    } else if (strcmp(layout, "sr_cyrillic") == 0) {
        sveska->current_layout = KEYBOARD_LAYOUT_SERBIAN_CYRILLIC;
    } else {
        sveska->current_layout = KEYBOARD_LAYOUT_US;
    }
}

void update_layout_indicator() {
    const char *layout_name;
    switch (sveska->current_layout) {
        case KEYBOARD_LAYOUT_SERBIAN_LATIN: layout_name = "SR Latin"; break;
        case KEYBOARD_LAYOUT_SERBIAN_CYRILLIC: layout_name = "SR Cyrillic"; break;
        default: layout_name = "EN"; break;
    }
    // Update your status bar or other UI element
    status_bar_set_text("Layout: %s", layout_name);
}
    */



    /** @}
 */
