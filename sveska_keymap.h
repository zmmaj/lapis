/** @addtogroup lapis
 * @{
 */
/**
 * @file
 */

#ifndef SVESKA_KEYMAP_H
#define SVESKA_KEYMAP_H

#include "sveska.h"  // Make sure this comes before any usage of utf8_char_t
#include <stdbool.h>
#include <io/kbd_event.h>

typedef struct sveska sveska_t;

bool is_special_combo(const kbd_event_t *event);
void handle_special_combo(sveska_t *sveska, const kbd_event_t *event);


#endif

/** @}
 */