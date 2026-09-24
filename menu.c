

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */

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


void update_font_size(sveska_t *sveska, float new_font_size);

typedef struct {
    sveska_t *sveska;
    size_t font_index;
} font_callback_data_t;


// Callback functions for menu entries
static void menu_action_new(ui_menu_entry_t *entry, void *arg);
static void menu_action_open(ui_menu_entry_t *entry, void *arg);
static void menu_action_save(ui_menu_entry_t *entry, void *arg);
static void menu_action_save_as(ui_menu_entry_t *entry, void *arg);
static void menu_action_exit(ui_menu_entry_t *entry, void *arg);
static void menu_action_undo(ui_menu_entry_t *entry, void *arg);
static void menu_action_redo(ui_menu_entry_t *entry, void *arg);
static void menu_action_bold(ui_menu_entry_t *entry, void *arg);
static void menu_action_italic(ui_menu_entry_t *entry, void *arg);
static void menu_action_underline(ui_menu_entry_t *entry, void *arg);
static void menu_action_insert_image(ui_menu_entry_t *entry, void *arg);

static void menu_action_font_selected(ui_menu_entry_t *entry, void *arg);




static void menu_action_size_1(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_2(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_3(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_4(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_5(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_6(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_7(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_8(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_9(ui_menu_entry_t *entry, void *arg);
static void menu_action_size_10(ui_menu_entry_t *entry, void *arg);


static void menu_action_color_red(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_green(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_blue(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_white(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_black(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_yellow(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_orange(ui_menu_entry_t *entry, void *arg);
static void menu_action_color_brown(ui_menu_entry_t *entry, void *arg);

static void menu_action_text_color_red(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_green(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_blue(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_white(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_black(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_yellow(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_orange(ui_menu_entry_t *entry, void *arg);
static void menu_action_text_color_brown(ui_menu_entry_t *entry, void *arg);

static void menu_action_shape_handwrite(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_line(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_circle(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_rectangle(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_circle_filled(ui_menu_entry_t *entry, void *arg);
static void menu_action_shape_rectangle_filled(ui_menu_entry_t *entry, void *arg);

static void menu_action_us_keyboard_layout(ui_menu_entry_t *entry, void *arg);
static void menu_action_sr_cir_keyboard_layout(ui_menu_entry_t *entry, void *arg);
static void menu_action_sr_lat_keyboard_layout(ui_menu_entry_t *entry, void *arg);

static void menu_action_help(ui_menu_entry_t *entry, void *arg);

static void file_open(void);

// Add these at the top with other function prototypes
static void open_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void open_dialog_bcancel(ui_file_dialog_t *, void *);
static void open_dialog_close(ui_file_dialog_t *, void *);

// Define the callback structure globally
static ui_file_dialog_cb_t open_dialog_cb = {
    .bok = open_dialog_bok,
    .bcancel = open_dialog_bcancel,
    .close = open_dialog_close
};

// Function to create menu bar
errno_t create_menu_bar(sveska_t *sveska) {
    errno_t rc;
    ui_menu_t *file_menu;
    ui_menu_t *edit_menu;
    ui_menu_t *font_menu;
    ui_menu_t *size_menu;
    ui_menu_t *color_menu;   // For Color menu
    ui_menu_t *color_text_menu;   // For Color menu
    ui_menu_t *shape_menu;   // For Shape menu
    ui_menu_t *keyboard_menu;   // For Shape menu
    ui_menu_t *help_menu;    // For Help menu

    ui_menu_entry_t *file_new_entry = NULL;
    ui_menu_entry_t *file_open_entry = NULL;
    ui_menu_entry_t *file_save_entry = NULL;
    ui_menu_entry_t *file_save_as_entry = NULL;
    ui_menu_entry_t *file_exit_entry = NULL;

   
    ui_menu_entry_t *action_undo_entry = NULL;
    ui_menu_entry_t *action_redo_entry = NULL;
    ui_menu_entry_t *action_bold_entry = NULL;
    ui_menu_entry_t *action_italic_entry = NULL;
    ui_menu_entry_t *action_underline_entry = NULL;
    ui_menu_entry_t *action_insert_image = NULL;



    ui_menu_entry_t *size_1_entry = NULL;
    ui_menu_entry_t *size_2_entry = NULL;
    ui_menu_entry_t *size_3_entry = NULL;
    ui_menu_entry_t *size_4_entry = NULL;
    ui_menu_entry_t *size_5_entry = NULL;
    ui_menu_entry_t *size_6_entry = NULL;
    ui_menu_entry_t *size_7_entry = NULL;
    ui_menu_entry_t *size_8_entry = NULL;
    ui_menu_entry_t *size_9_entry = NULL;
    ui_menu_entry_t *size_10_entry = NULL;
    

    ui_menu_entry_t *color_text_red_entry = NULL;    // Red color_text entry
    ui_menu_entry_t *color_text_green_entry = NULL;    // Green color_text entry
    ui_menu_entry_t *color_text_blue_entry = NULL;     // Blue color_text entry
    ui_menu_entry_t *color_text_white_entry = NULL;    // White color_text entry
    ui_menu_entry_t *color_text_black_entry = NULL;    // Black color_text entry
    ui_menu_entry_t *color_text_yellow_entry = NULL;   // Yellow color_text entry
    ui_menu_entry_t *color_text_orange_entry = NULL;   // Orange color_text entry
    ui_menu_entry_t *color_text_brown_entry = NULL;    // Brown color_text entry

    ui_menu_entry_t *color_red_entry = NULL;    // Red color entry
    ui_menu_entry_t *color_green_entry = NULL;    // Green color entry
    ui_menu_entry_t *color_blue_entry = NULL;     // Blue color entry
    ui_menu_entry_t *color_white_entry = NULL;    // White color entry
    ui_menu_entry_t *color_black_entry = NULL;    // Black color entry
    ui_menu_entry_t *color_yellow_entry = NULL;   // Yellow color entry
    ui_menu_entry_t *color_orange_entry = NULL;   // Orange color entry
    ui_menu_entry_t *color_brown_entry = NULL;    // Brown color entry

    ui_menu_entry_t *shape_handwrite_entry = NULL;
    ui_menu_entry_t *shape_line_entry = NULL;
    ui_menu_entry_t *shape_circle_entry = NULL;
    ui_menu_entry_t *shape_circle_filled_entry = NULL;
    ui_menu_entry_t *shape_rectangle_entry = NULL;
    ui_menu_entry_t *shape_rectangle_filled_entry = NULL;

    ui_menu_entry_t *us_keyboard_layout = NULL;
    ui_menu_entry_t *sr_cir_keyboard_layout = NULL;
    ui_menu_entry_t *sr_lat_keyboard_layout = NULL;

    ui_menu_entry_t *help_entry = NULL;  // Help menu entry



    gfx_rect_t rectm;
    rectm.p0.x = 4;
    rectm.p0.y = 30;
    rectm.p1.x = sveska->window_width - 4;
    rectm.p1.y = 52;

    /* Create the menu bar */
    rc = ui_menu_bar_create(sveska->ui, sveska->window, &sveska->menubar);
    if (rc != EOK) {
        printf("Greska pri kreiranju menu bar.\n");
        return rc;
    }
    ui_menu_bar_set_rect(sveska->menubar, &rectm);



    /* Create Document menu */
    rc = ui_menu_dd_create(sveska->menubar, "~D~okument", NULL, &file_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Document menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(file_menu, "Novo", "Ctrl+N", &file_new_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju NEW menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_new_entry, menu_action_new, (void *)sveska);

    rc = ui_menu_entry_create(file_menu, "Otvori", "Ctrl+O", &file_open_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Open menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_open_entry, menu_action_open, sveska);

    rc = ui_menu_entry_create(file_menu, "Sacuvaj", "Ctrl+S", &file_save_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Save menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_save_entry, menu_action_save, sveska);

    rc = ui_menu_entry_create(file_menu, "Sacuvaj kao", "Ctrl+Shift+S", &file_save_as_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Save As menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_save_as_entry, menu_action_save_as, sveska);

    rc = ui_menu_entry_create(file_menu, "Izlaz", "Ctrl+Q", &file_exit_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Exit menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(file_exit_entry, menu_action_exit, NULL);




    /* Create Actions menu */
    rc = ui_menu_dd_create(sveska->menubar, "~A~kcije", NULL, &edit_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Actions menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(edit_menu, "Undo", "Ctrl+Z", &action_undo_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Undo menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_undo_entry, menu_action_undo, sveska);

    rc = ui_menu_entry_create(edit_menu, "Redo", "Ctrl+Y", &action_redo_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Redo menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_redo_entry, menu_action_redo, sveska);

    rc = ui_menu_entry_create(edit_menu, "bold", "Ctrl+Z", &action_bold_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Bold menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_bold_entry, menu_action_bold, sveska);


    rc = ui_menu_entry_create(edit_menu, "Italic", "Ctrl+Z", &action_italic_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju italic menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_italic_entry, menu_action_italic, sveska);


    rc = ui_menu_entry_create(edit_menu, "Podvuceno", "Ctrl+Z", &action_underline_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Podvuceno menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_underline_entry, menu_action_underline, sveska);

  
    rc = ui_menu_entry_create(edit_menu, "Ubaci sliku", "Ctrl+I", &action_insert_image);
    if (rc != EOK) {
        printf("Greska pri kreiranju Ubaci sliku menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(action_insert_image, menu_action_insert_image, sveska);



    /* Create font menu ***************************************************/

    rc = ui_menu_dd_create(sveska->menubar, "~F~ont", NULL, &font_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Font menu.\n");
        return rc;
    }
    
    printf("Total fonts read from list: %zu\n", sveska->font_count);
    
    for (size_t i = 0; i < sveska->font_count; i++) {
        ui_menu_entry_t *entry;
    
        // Prepare a clean display name (strip .ttf if present)
        char entry_name[MAX_FONT_NAME_LEN];
        str_cpy(entry_name, sizeof(entry_name), sveska->font_names[i]);
    
        // Strip .ttf or .TTF extension for cleaner UI
        char *ext = str_rchr(entry_name, '.');  // use your existing custom str_rchr
        if (ext && str_casecmp(ext, ".ttf") == 0) {
            *ext = '\0';  // remove extension
        }
    
        // Create menu entry with only the cleaned font name as label
        // Pass empty string "" instead of NULL for description to be safe
        rc = ui_menu_entry_create(font_menu, entry_name, "", &entry);
        if (rc != EOK) {
            printf("Greska pri kreiranju menu entry for font: %s\n", sveska->font_names[i]);
            continue;
        }
    
        // Allocate callback data
        font_callback_data_t *data = malloc(sizeof(font_callback_data_t));
        if (data == NULL) {
            printf("Greska pri alociranju memory for font_cb_data\n");
            continue;
        }
    
        data->sveska = sveska;
        data->font_index = i;
    
        ui_menu_entry_set_cb(entry, menu_action_font_selected, data);
    }
    

    
    

    /* Create Size menu **************************************************/
    rc = ui_menu_dd_create(sveska->menubar, "~V~elicina", NULL, &size_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(size_menu, "8", "Ctrl+1", &size_1_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 8 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_1_entry, menu_action_size_1, sveska);

    rc = ui_menu_entry_create(size_menu, "10", "Ctrl+2", &size_2_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 10 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_2_entry, menu_action_size_2, sveska);

    rc = ui_menu_entry_create(size_menu, "12", "Ctrl+3", &size_3_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 12 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_3_entry, menu_action_size_3, sveska);

    rc = ui_menu_entry_create(size_menu, "14", "Ctrl+4", &size_4_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 14 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_4_entry, menu_action_size_4, sveska);

    rc = ui_menu_entry_create(size_menu, "16", "Ctrl+5", &size_5_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 16 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_5_entry, menu_action_size_5, sveska);
   


    rc = ui_menu_entry_create(size_menu, "18", "Ctrl+5", &size_6_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 18 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_6_entry, menu_action_size_6, sveska);


    rc = ui_menu_entry_create(size_menu, "20", "Ctrl+7", &size_7_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 20 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_7_entry, menu_action_size_7, sveska);


    rc = ui_menu_entry_create(size_menu, "22", "Ctrl+8", &size_8_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 22 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_8_entry, menu_action_size_8, sveska);



    rc = ui_menu_entry_create(size_menu, "24", "Ctrl+9", &size_9_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 24 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_9_entry, menu_action_size_9, sveska);


    rc = ui_menu_entry_create(size_menu, "26", "Ctrl+0", &size_10_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Size 26 menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(size_10_entry, menu_action_size_10, sveska);

  /******************* kreiramo  boju texta */

 /* Create Color menu */
 rc = ui_menu_dd_create(sveska->menubar, "~B~oja teksta", NULL, &color_text_menu);
 if (rc != EOK) {
     printf("Greska pri kreiranju Color menu.\n");
     return rc;
 }

 rc = ui_menu_entry_create(color_text_menu, "Crvena", "Ctrl+R", &color_text_red_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Red color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_red_entry, menu_action_text_color_red, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Zelena", "Ctrl+G", &color_text_green_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Green color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_green_entry, menu_action_text_color_green, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Plava", "Ctrl+B", &color_text_blue_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Blue color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_blue_entry, menu_action_text_color_blue, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Bela", "Ctrl+W", &color_text_white_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju White color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_white_entry, menu_action_text_color_white, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Crna", "Ctrl+K", &color_text_black_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Black color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_black_entry, menu_action_text_color_black, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Zuta", "Ctrl+Shift+Y", &color_text_yellow_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Yellow color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_yellow_entry, menu_action_text_color_yellow, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Narandzasta", "Ctrl+Shift+O", &color_text_orange_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Orange color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_orange_entry, menu_action_text_color_orange, sveska);

 rc = ui_menu_entry_create(color_text_menu, "Braon", "Ctrl+N", &color_text_brown_entry);
 if (rc != EOK) {
     printf("Greska pri kreiranju Brown color menu entry.\n");
     return rc;
 }
 ui_menu_entry_set_cb(color_text_brown_entry, menu_action_text_color_brown, sveska);


 /************************************************************************* */


    /* Create Color menu */
    rc = ui_menu_dd_create(sveska->menubar, "~B~oja pozadine", NULL, &color_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Color menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(color_menu, "Crvena", "Ctrl+R", &color_red_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Red color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_red_entry, menu_action_color_red, sveska);

    rc = ui_menu_entry_create(color_menu, "Zelena", "Ctrl+G", &color_green_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Green color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_green_entry, menu_action_color_green, sveska);

    rc = ui_menu_entry_create(color_menu, "Plava", "Ctrl+B", &color_blue_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Blue color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_blue_entry, menu_action_color_blue, sveska);

    rc = ui_menu_entry_create(color_menu, "Bela", "Ctrl+W", &color_white_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju White color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_white_entry, menu_action_color_white, sveska);

    rc = ui_menu_entry_create(color_menu, "Crna", "Ctrl+K", &color_black_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Black color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_black_entry, menu_action_color_black, sveska);

    rc = ui_menu_entry_create(color_menu, "Zuta", "Ctrl+Shift+Y", &color_yellow_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Yellow color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_yellow_entry, menu_action_color_yellow, sveska);

    rc = ui_menu_entry_create(color_menu, "Narandzasta", "Ctrl+Shift+O", &color_orange_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Orange color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_orange_entry, menu_action_color_orange, sveska);

    rc = ui_menu_entry_create(color_menu, "Braon", "Ctrl+N", &color_brown_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Brown color menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(color_brown_entry, menu_action_color_brown, sveska);









    /* Create Shape menu *******************************************/
    rc = ui_menu_dd_create(sveska->menubar, "~O~blik", NULL, &shape_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Shape menu.\n");
        return rc;
    }

    rc = ui_menu_entry_create(shape_menu, "Slobodno", "Ctrl+H", &shape_handwrite_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Handwrite shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_handwrite_entry, menu_action_shape_handwrite, sveska);

    rc = ui_menu_entry_create(shape_menu, "Linija", "Ctrl+L", &shape_line_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Line shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_line_entry, menu_action_shape_line, sveska);

    rc = ui_menu_entry_create(shape_menu, "Krug", "Ctrl+C", &shape_circle_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Circle shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_circle_entry, menu_action_shape_circle, sveska);

    rc = ui_menu_entry_create(shape_menu, "Pun Krug", "Ctrl+D", &shape_circle_filled_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Filled Circle shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_circle_filled_entry, menu_action_shape_circle_filled, sveska);


    rc = ui_menu_entry_create(shape_menu, "Pun Cetvorougao", "Ctrl+F", &shape_rectangle_filled_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju FILLED Rectangle shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_rectangle_filled_entry, menu_action_shape_rectangle_filled, sveska);

    rc = ui_menu_entry_create(shape_menu, "Cetvorougao", "Ctrl+R", &shape_rectangle_entry);
    if (rc != EOK) {
        printf("Greska pri kreiranju Rectangle shape menu entry.\n");
        return rc;
    }
    ui_menu_entry_set_cb(shape_rectangle_entry, menu_action_shape_rectangle, sveska);

/*              tastatura izbor  *****************************/

rc = ui_menu_dd_create(sveska->menubar, "~T~astatura", NULL, &keyboard_menu);
if (rc != EOK) {
    printf("Greska pri kreiranju Shape menu.\n");
    return rc;
}


rc = ui_menu_entry_create(keyboard_menu, "USA Tastatura", "Ctrl+R", &us_keyboard_layout);
if (rc != EOK) {
    printf("Greska pri kreiranju us_keyboard_layout.\n");
    return rc;
}
ui_menu_entry_set_cb(us_keyboard_layout, menu_action_us_keyboard_layout, sveska);



rc = ui_menu_entry_create(keyboard_menu, "Ci Tastatura", "Ctrl+R", &sr_cir_keyboard_layout);
if (rc != EOK) {
    printf("Greska pri kreiranju sr_cir_keyboard_layout.\n");
    return rc;
}
ui_menu_entry_set_cb(sr_cir_keyboard_layout, menu_action_sr_cir_keyboard_layout, sveska);


rc = ui_menu_entry_create(keyboard_menu, "Lat Tastatura", "Ctrl+R", &sr_lat_keyboard_layout);
if (rc != EOK) {
    printf("Greska pri kreiranju sr_lat_keyboard_layout.\n");
    return rc;
}
ui_menu_entry_set_cb(sr_lat_keyboard_layout, menu_action_sr_lat_keyboard_layout, sveska);



    /* Create Help menu */
    rc = ui_menu_dd_create(sveska->menubar, "~P~omoc", NULL, &help_menu);
    if (rc != EOK) {
        printf("Greska pri kreiranju Help menu.\n");
        return rc;
    }
  printf("Pre Help menu entry.\n");
    rc = ui_menu_entry_create(help_menu, "Pomoc", "F1", &help_entry);
    if (rc != EOK) {
       printf("Greska pri kreiranju Help menu entry.\n");
        return rc;
    }

    ui_menu_entry_set_cb(help_entry, menu_action_help, sveska);
  printf("zavrsio cb.\n");

  rc = ui_fixed_add(sveska->fixed, ui_menu_bar_ctl(sveska->menubar));
  if (rc != EOK) {
      printf("Greska pri dodavanja kontrola izgledu.\n");
      return rc;
  }

    return EOK;
}

// Action callbacks for menu entries
void menu_action_new(ui_menu_entry_t *entry, void *arg) {
    printf("NEW action triggered\n");

}

// Action callbacks for menu entries
void menu_action_open(ui_menu_entry_t *entry, void *arg) {
    // For now, hardcode the file path

    printf("krece otvaranje dijaloga.\n");
    file_open();
    printf("zavrsio otvaranje dijaloga.\n");
  /*
    const char *path = "novi_text.zmj";
    
    if (load_document(sveska, path)) {
        printf("Document loaded successfully\n");
    } else {
        // Show error message
        printf("Ne mogu da ucitam dokument\n");
    }
        */
        return;
}

void menu_action_save(ui_menu_entry_t *entry, void *arg) {
    if (sveska->current_file[0] != '\0') {
        // Save to existing file
        save_document(sveska, sveska->current_file);
    } else {
        // No file selected, use default name
        save_document(sveska, "novi_text.zmj");
    }
}


void menu_action_save_as(ui_menu_entry_t *entry, void *arg) {
        // For now, just save with a different name
        save_document(sveska, "novi_text_2.zmj");
}

void menu_action_exit(ui_menu_entry_t *entry, void *arg) {
    printf("Exit action triggered\n");
    exit(0); // Exit the application
}


void menu_action_undo(ui_menu_entry_t *entry, void *arg) {
        sveska_t *sveska = (sveska_t *)arg;
        perform_undo(sveska);

}

void menu_action_redo(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    perform_redo(sveska);

}

void menu_action_bold(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    if (!sveska) return;

    // Case 1: No selection - toggle bold for new text
    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
        sveska->bold = !sveska->bold;
        return;
    }

    // Case 2: Text is selected - toggle bold for selection
    toggle_bold_selected(sveska);
}


void menu_action_italic(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    if (!sveska) return;

    // Toggle the italic flag
    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
    sveska->italic = !sveska->italic;
    return;
    }

        toggle_italic_selected(sveska);

}



void menu_action_underline(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    if (!sveska) return;

    // Toggle the underline flag
    if (sveska->selection.start_pos == -1 || sveska->selection.end_pos == -1) {
    sveska->underline = !sveska->underline;
    return;
}

    // If there's a selection, apply to selection
    if (has_selection(sveska)) {
        toggle_underline_selected(sveska);
    }
    // No selection case is handled automatically in handle_keypress
}


void menu_action_insert_image(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    if (!sveska) return;

    insert_image_placeholder(sveska);  
}


//*     FONT ***************************************************/
// Font callbacks for menu entries
static void menu_action_font_selected(ui_menu_entry_t *entry, void *arg) {
    font_callback_data_t *data = (font_callback_data_t *)arg;
    if (!data || !data->sveska) return;

    sveska_t *sveska = data->sveska;
    size_t font_index = data->font_index;

    if (font_index >= sveska->font_count) return;

    sveska_font_t *new_font = &sveska->fonts[font_index];
    
    // Case 1: Change font for selected text only
    if (has_selection(sveska)) {
        set_font_selected(sveska, new_font->path, sveska->font_size);
    } 
    // Case 2: Change font for future text
    else {
        sveska->font = new_font;
        sveska->current_font_index = font_index;
        
        // Create new span only if current span has content
        if (sveska->document.count > 0) {
            text_span_t *last_span = &sveska->document.spans[sveska->document.count-1];
            if (last_span->length > 0) {
                create_new_span(sveska);
            } else {
                // Update empty span's font
                last_span->font = new_font;
                last_span->font_size = sveska->font_size;
                str_ncpy(last_span->font_path, MAX_FONT_PATH_LEN, 
                        new_font->path, str_size(new_font->path));
            }
        }
        
        rebuild_display_text(sveska);
        update_cursor_position(sveska);
        sveska_text_render(sveska);
    }
}













/*          END FONT ***************************************/

// Size callbacks for menu entries
void menu_action_size_1(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 16.0f);
}

void menu_action_size_2(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 18.0f);
}

void menu_action_size_3(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 20.0f);
}

void menu_action_size_4(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 22.0f);
}

void menu_action_size_5(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 24.0f);
}

void menu_action_size_6(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 26.0f);
}


void menu_action_size_7(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 28.0f);
}


void menu_action_size_8(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 30.0f);
}



void menu_action_size_9(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 32.0f);
}



void menu_action_size_10(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    update_font_size(sveska, 34.0f);
}


// Color callbacks for menu entries
void menu_action_color_red(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0x0000, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska);  

}

void menu_action_color_green(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0xFFFF, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska);  

}

void menu_action_color_blue(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0x0000, 0xFFFF, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

void menu_action_color_white(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xFFFF, 0xFFFF, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

void menu_action_color_black(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0x0000, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

void menu_action_color_yellow(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xFFFF, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

void menu_action_color_orange(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xA500, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

void menu_action_color_brown(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xA52A, 0x2A2A, 0x0000, &sveska->bkg_color);
    sveska_text_render(sveska); 

}

// Shape callbacks for menu entries
void menu_action_shape_handwrite(ui_menu_entry_t *entry, void *arg) {

}

void menu_action_shape_line(ui_menu_entry_t *entry, void *arg) {
}

void menu_action_shape_circle(ui_menu_entry_t *entry, void *arg) {

}

void menu_action_shape_circle_filled(ui_menu_entry_t *entry, void *arg) {

}

void menu_action_shape_rectangle(ui_menu_entry_t *entry, void *arg) {


}
void menu_action_shape_rectangle_filled(ui_menu_entry_t *entry, void *arg) {

}
/***************************************************************** */

/************  TEXT COLOR  ******************** */
// Color callbacks for menu entries
void menu_action_text_color_red(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0x0000, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_green(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0xFFFF, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_blue(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0x0000, 0xFFFF, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_white(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xFFFF, 0xFFFF, &sveska->bkg_color);
    sveska_text_render(sveska);

}

void menu_action_text_color_black(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0x0000, 0x0000, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_yellow(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xFFFF, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_orange(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xFFFF, 0xA500, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

void menu_action_text_color_brown(ui_menu_entry_t *entry, void *arg) {
    gfx_color_new_rgb_i16(0xA52A, 0x2A2A, 0x0000, &sveska->color);
    sveska_text_render(sveska);

}

/*
    ui_menu_entry_t *us_keyboard_layout = NULL;
    ui_menu_entry_t *sr_cir_keyboard_layout = NULL;
    ui_menu_entry_t *sr_lat_keyboard_layout = NULL;

*/

/*   TASTATURAAA *************/
void menu_action_us_keyboard_layout(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    sveska->current_layout = KEYBOARD_LAYOUT_US;
    printf("Keyboard layout set to US\n");
    // Optional: Update status bar or other UI feedback
}

void menu_action_sr_cir_keyboard_layout(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    sveska->current_layout = KEYBOARD_LAYOUT_SERBIAN_CYRILLIC;
    printf("Keyboard layout set to Serbian Cyrillic\n");
    // Optional: Update status bar or other UI feedback

}

void menu_action_sr_lat_keyboard_layout(ui_menu_entry_t *entry, void *arg) {
    sveska_t *sveska = (sveska_t *)arg;
    sveska->current_layout = KEYBOARD_LAYOUT_SERBIAN_LATIN;
    printf("Keyboard layout set to Serbian Latin\n");
    // Optional: Update status bar or other UI feedback

}





void menu_action_help(ui_menu_entry_t *entry, void *arg)
{    
    return;
}




void update_font_size(sveska_t *sveska, float new_font_size) {
    // Step 1: Update the font size in the sveska structure
    sveska->font_size = new_font_size;
    printf("Velicina fonta promenjena na %.1f\n", new_font_size);

    // Step 2: If text is selected, change the font size in the selected range of spans
    if (sveska->selection.is_selecting) {
        size_t start = sveska->selection.start_pos;
        size_t end = sveska->selection.end_pos;

        printf("Menjam velicinu fonta za selektovan text (start: %zu, end: %zu)\n", start, end);

        // Iterate over the selected range of spans and update the font size
        for (size_t i = start; i < end; ++i) {
            text_span_t *span = &sveska->document.spans[i];
          //  printf("Before: Span %zu font size: %.1f\n", i, span->font_size);

            span->font_size = sveska->font_size;  // Update font size for selected text

           // printf("After: Span %zu font size: %.1f\n", i, span->font_size);
        }
    } else {
        // Step 3: If no text is selected, create a new span with the updated font size
        // Start by checking if we need to expand the spans array
        if (sveska->document.count >= sveska->document.capacity) {
            sveska->document.capacity *= 2;
            sveska->document.spans = realloc(sveska->document.spans, sveska->document.capacity * sizeof(text_span_t));
        }

        // Create a new span for the current text line
        sveska->current_span = sveska->document.count;
        text_span_t *new_span = &sveska->document.spans[sveska->current_span];
        new_span->capacity = 256;  // Initial capacity for the span's text
        new_span->text = malloc(new_span->capacity);
        if (!new_span->text) {
            printf("Neuspela alokacija memorije za novi span texta\n");
            return;
        }

        new_span->length = 0;
        new_span->text[0] = '\0';  // Empty string for now
        new_span->bold = sveska->bold;
        new_span->italic = sveska->italic;
        new_span->underline = sveska->underline;
        new_span->font = sveska->font ? sveska->font : NULL;
        new_span->font_size = sveska->font_size;  // New font size

        // Increment the document span count
        sveska->document.count++;

        printf("Novi span kreiran sa vrlicinom fonta %.1f\n", sveska->font_size);
    }

    // Step 4: Update the font size in the font struct, so future text uses it
    if (sveska->font) {
        sveska->font->size = sveska->font_size;
    }

    // Step 5: Trigger re-render of the document with the new font size
    sveska_text_render(sveska);
}


/** Open Open File dialog. */
static void file_open(void)
{
	ui_file_dialog_params_t fdparams;
	ui_file_dialog_t *dialog;
	errno_t rc;

	ui_file_dialog_params_init(&fdparams);
	fdparams.caption = "Open File";
//	fdparams.ifname = old_fname;

	rc = ui_file_dialog_create(sveska->ui, &fdparams, &dialog);
	if (rc != EOK) {
		printf("Greska pri kreiranju message dialog.\n");
		return;
	}

	ui_file_dialog_set_cb(dialog, &open_dialog_cb, sveska);
}



static void open_dialog_bok(ui_file_dialog_t *dialog, void *arg, const char *fname)
{
    sveska_t *sveska = (sveska_t *)arg;
    char *cname = str_dup(fname);  // Normalize and format the path
    
    if (cname == NULL) {
        printf("Van memorije.\n");
        ui_file_dialog_destroy(dialog);
        return;
    }

    const char *ext = daj_extenziju(cname);  // Use normalized path for extension check

    // Handle different file types
    if (str_casecmp(ext, ".txt") == 0) {
        printf("This is a TXT file: %s\n", cname);
        open_txt_file(sveska, cname);  // Use normalized path
    } 
    else if (str_casecmp(ext, ".zmj") == 0) {
        printf("This is a ZMJ file: %s\n", cname);
        load_document(sveska, cname);  // Use normalized path
    }
    else {
        printf("Unsupported file type: %s\n", ext);
    }

    // Free the normalized path
    free(cname);
    ui_file_dialog_destroy(dialog);
    // Refresh display
    gfx_update(ui_window_get_gc(sveska->window));
}


static void open_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	sveska_t *sveska = (sveska_t *)arg;

	(void)sveska;
	ui_file_dialog_destroy(dialog);
}


/** Open File dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
 static void open_dialog_close(ui_file_dialog_t *dialog, void *arg)
 {
	sveska_t *sveska = (sveska_t *)arg;

	(void)sveska;
     ui_file_dialog_destroy(dialog);
 }

/** @}
 */

