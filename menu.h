
#include <ui/ui.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/msgdialog.h>
#include <ui/pbutton.h>
#include <ui/control.h>
#include <ui/paint.h>

#include <gfx/render.h>
#include <ui/control.h>
#include <ui/filedialog.h>
#include <ui/fixed.h>
#include <types/common.h>
#include <ui/control.h>
#include <ui/promptdialog.h>
#include <ui/resource.h>

#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include "sveska.h"


// Callback functions for menu entries
extern void menu_action_new(ui_menu_entry_t *entry, void *arg);
extern void menu_action_open(ui_menu_entry_t *entry, void *arg);
extern void menu_action_save(ui_menu_entry_t *entry, void *arg);
extern void menu_action_save_as(ui_menu_entry_t *entry, void *arg);






extern void file_open(void);
