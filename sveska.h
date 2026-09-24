

/** @addtogroup lapis
 * @{
 */
/**
 * @file
 */


#ifndef SVESKA_H
#define SVESKA_H

/* Type-safe MIN/MAX macros */
#define MAX(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    (void)(&_a == &_b); /* Type compatibility check */ \
    _a > _b ? _a : _b; \
})

#define MIN(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    (void)(&_a == &_b); /* Type compatibility check */ \
    _a < _b ? _a : _b; \
})

/* Size-specific versions for size_t comparisons */
#define MAX_SIZE(a, b) ({ \
    size_t _a = (a); \
    size_t _b = (b); \
    _a > _b ? _a : _b; \
})

#define MIN_SIZE(a, b) ({ \
    size_t _a = (a); \
    size_t _b = (b); \
    _a < _b ? _a : _b; \
})




/* Add these near your MIN/MAX macros */
#define CLAMP(val, min, max) ({ \
    __typeof__(val) _val = (val); \
    __typeof__(min) _min = (min); \
    __typeof__(max) _max = (max); \
    (_val < _min) ? _min : ((_val > _max) ? _max : _val); \
})

#include <str.h>
#include <stdlib.h>
#include <clipboard.h>
#include <gfx/context.h>
#include <gfx/bitmap.h>
#include <ui/window.h>
#include <ui/ui.h>
#include <ui/image.h>
#include <ui/filedialog.h>
#include <ui/wdecor.h>
#include <ui/paint.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/msgdialog.h>
#include <ui/resource.h>
#include <ui/paint.h>
#include <ui/pbutton.h>
#include <ui/fixed.h>
#include "sveska_font.h"
#include "sveska_keymap.h"

#define MAX_FONTS 64
#define MAX_FONT_NAME_LEN 128
#define MAX_FONT_PATH_LEN 256
#define MAX_PATH_LENGTH 256
#define MAX_SPANS 1024  // Maximum number of text spans
#define INITIAL_SPAN_CAPACITY 256
#define UTF8_MAX_BYTES 4

#define MAX_FONT_CACHE 64


typedef struct {
    char path[MAX_FONT_PATH_LEN];
    float size;
    sveska_font_t *font;
} font_cache_entry_t;

typedef enum {
    KEYBOARD_LAYOUT_US,
    KEYBOARD_LAYOUT_SERBIAN_LATIN,
    KEYBOARD_LAYOUT_SERBIAN_CYRILLIC
} keyboard_layout_t;

typedef struct {
    uint8_t bytes[UTF8_MAX_BYTES];
    size_t length; // Number of bytes (1-4)
} utf8_char_t;

/* Character position tracking structure */
typedef struct {
    int x;        // X position (left)
    int y;        // Y position (baseline)
    int width;    // Character width
    int height;   // Character height
} char_position_t;

typedef struct {
    int start_idx;  // Starting character index
    int end_idx;    // Ending character index
    int y_pos;      // Line Y position

    int y_top;      // Line top position
    int y_bottom; 
} text_line_t;


/* Forward declaration for self-referential types */
typedef struct sveska sveska_t;

typedef struct {
    int keycode;
    char normal_char;
    char shifted_char;
} keymap_debug_t;


typedef struct {
    int x;
    int y;
} mouse_pos_t;

typedef struct {
    int x;
    int y;
    int height;
    int width;
    bool visible;
} char_cursor_t;

typedef struct {

    size_t byte_length; // Actual bytes used
    size_t char_length; // Number of UTF-8 characters


    char *text;  // UTF-8 encoded text
    size_t length;
    size_t capacity;

    // Font information (now mandatory for each span)
    sveska_font_t *font;      // Never NULL after initialization
    int font_index;  
    float font_size;          // Always set
    char font_path[MAX_FONT_PATH_LEN]; // Store path for serialization
    
    uint16_t color; 
    bool bold;
    bool italic;
    bool underline;

    // Optional per-character style arrays (can be NULL if unused)
    bool *bold_array;
    bool *italic_array;
    bool *underline_array;
 
    int y_pos; 
//podrska za sliku
bool is_image;
gfx_bitmap_t *bitmap;
gfx_bitmap_params_t bitmap_params;
int img_width;
int img_height;

} text_span_t;


typedef struct {
    text_span_t *spans;
    size_t count;
    size_t capacity;
} text_document_t;

typedef struct {
    int start_pos;
    int end_pos;
    bool is_selecting;  // Track if the user is currently selecting
    gfx_coord2_t mouse_pos;
    gfx_coord2_t press_pos; 
    int prev_codepoint;
} text_selection_t;

typedef enum {
    EDIT_INSERT,
    EDIT_DELETE
} edit_type_t;


// In sveska.h
typedef enum {
    OP_INSERT,
    OP_DELETE,
    OP_FORMAT,  // For bold/italic/underline changes
    OP_FONT_CHANGE, // for font change support
    OP_SPAN_SPLIT,  // When a span is split due to formatting
    OP_SPAN_MERGE,
    OP_SPAN_REMOVE  // New type for span removal
} operation_type_t;

typedef struct {
    operation_type_t type;  // Ensure it's 'operation_type_t'
    char character;
    size_t position;
    size_t span_index;  // Changed from uint64_t timestamp to size_t span_index
    uint64_t timestamp; 
} edit_history_entry_t;



typedef enum {
    FORMAT_BOLD = 0,
    FORMAT_ITALIC = 1,
    FORMAT_UNDERLINE = 2
} format_attribute_t;


typedef struct {
    operation_type_t type;
    union {
        struct {
            char *text;        // For text insertion/deletion
            size_t position;   // Global document position
            size_t length;
            char font_path[MAX_FONT_PATH_LEN]; // font change support
            float font_size;   //  font change support
        } text;
        struct {
            size_t span_index;
            bool old_value;
            bool new_value;
            size_t position_in_span;
            uint8_t format_type; // 0=bold, 1=italic, 2=underline
            char font_path[MAX_FONT_PATH_LEN]; // For font changes
            float font_size;    // For font changes
        } format;
        struct {
            size_t span_index;
            size_t split_pos;
        } span;
    } data;
    size_t affected_span;     // Which span was affected
    uint64_t timestamp;
    void *user_data;  // For storing additional operation data
} edit_operation_t;



typedef struct {
    char *text;         // Text content
    bool *bold;         // Parallel array of bold flags
    bool *italic;       // Parallel array of italic flags
    bool *underline;    // Parallel array of underline flags
    size_t length;      // Length of text
    char font_path[MAX_FONT_PATH_LEN]; // Font path
    float font_size;    // Font size
    size_t version;
} clipboard_data_t;



/* Complete structure definition */
struct sveska {
    /* Core UI elements */
    ui_t *ui;
    ui_window_t *window;
    gfx_context_t *window_gc;
       	/** UI resource */
	ui_resource_t *ui_res;
    gfx_color_t *color;
    gfx_color_t *bkg_color;
    
    gfx_color_t *highlight_color;
    gfx_rect_t  *bkg_rect;
    
    /* Text editing */
    char *text_buffer;
    size_t text_length;
    size_t text_capacity;
    sveska_font_t *font;
    float font_size;
    int char_spacing;
    int cursor_x;
    int cursor_y;
    int line_height;
    int window_width;
    int window_height;
    float scale;
   
    bool bold;          // Add this flag
    bool italic;        // For future use
    bool underline; 
    bool fullscreen;

    /* Image support */
    size_t imgs_count;
    size_t imgs_current;
    char **imgs;
    gfx_bitmap_t **img_bitmaps;
    gfx_rect_t *img_rects;
    gfx_bitmap_t *bitmap;
    ui_image_t *image;
    
    /* Dialog support */
    ui_file_dialog_t *dialog;
    
    /* parametri Prozora */
    ui_wnd_params_t *params;
    gfx_bitmap_params_t *bparams;
    
    /* Keyboard mapping */
    keymap_debug_t *keymap_debug;
    size_t keymap_debug_count;
    int margin_x;          // Left/right margin
    int margin_y;          // Top margin
    int chars_per_line;    // Max chars per line
    
    /* Character tracking */
    gfx_color_t *bg_color;
    char_position_t *char_positions;
    size_t char_pos_count;
    text_line_t *lines;
   // size_t line_count;

    /*MENI*/
    ui_menu_bar_t *menubar;
    ui_menu_bar_cb_t *meni;
    ui_menu_t *menu;
    ui_menu_t *edit;
    ui_menu_t *view;
    ui_menu_t *help;
    gfx_rect_t rect;
    ui_fixed_t *fixed;

    stbtt_fontinfo info;
    unsigned char *font_data;
    char font_names[MAX_FONTS][MAX_FONT_NAME_LEN];
    char current_font_path[MAX_FONT_PATH_LEN];
    size_t font_count;
    size_t current_font_index;
    sveska_font_t *current_font;
    font_cache_entry_t font_cache[MAX_FONT_CACHE];
    size_t font_cache_count;
    sveska_font_t fonts[MAX_FONTS]; 


    /* Mouse position */
    mouse_pos_t mouse_pos; 
    int cursor_get_pos;
    int cursor_move;
    int cursor_index; 
    /*text obelezavanje*/
    text_selection_t selection;
    text_document_t document;
    size_t current_span;  // Index of current active span
   

    char_cursor_t char_cursor;
    bool force_cursor_position; 

    edit_operation_t *edit_history;  // Not edit_history_entry_t
    size_t history_size;
    size_t history_capacity;
    size_t history_pos;  // Current position in history (for undo/redo)
    bool in_undo_redo;   // Flag to prevent recording during undo/redo

    /***** prevent cursor setting while pasting text */
   bool paste;
   int ascent;

   int text_margin_x;
   int text_margin_y;
   int text_area_offset_y;
   int menu_bar_height;           // Variable instead of #define
   int menu_bar_number;

   int last_line_height; 
   int current_line_height; 

   bool debug_enabled;

   int *line_y;         // Array of Y positions for each line
   size_t new_line_count;   // Number of new lines
   size_t line_cap;     // Array capacity
   int *line_y_positions;
   int trenutna_linija;

   size_t (*utf8_char_length)(const char *);
   utf8_char_t (*get_utf8_char)(const char *, size_t);
   int (*utf8_strlen)(const char *);

   keyboard_layout_t current_layout;
   int current_line_baseline;
   int current_line_top;

   size_t line_count;    // Number of lines in the document


   int last_cursor_x; // Track last x position for vertical movement
   bool cursor_visible; // Track cursor visibility

   //int *line_y_positions;   // Array of Y positions for each line (keep existing name)
   int *line_heights;       // NEW: Array of heights for each line
   int char_width; 
   bool skip_cursor_update;
   bool caps_lock_on;
   bool overwrite_mode;
   bool is_quitting; 
 // iniciram cuvanje i modifikaciju dokumenta
 char current_file[MAX_PATH_LENGTH];  // Trenutna putanja fajla
 bool document_modified;           // Pratim nesacuvane promene dokumenta
};

#ifdef SVESKA_MAIN
#define EXTERN 
#else 
#define EXTERN extern
#endif

EXTERN sveska_t *sveska;




/* Function declarations */
extern void sveska_text_render(sveska_t *sveska);
void sveska_window_destroy(sveska_t *sveska);
bool sveska_img_setup(sveska_t *sveska, size_t img_idx);
bool sveska_img_load(sveska_t *sveska, const char *fname, size_t img_idx);
extern void sveska_record_keypress(sveska_t *sveska, kbd_event_t *event);
extern bool is_font_file(const char *filename) ;
extern errno_t scan_and_save_font_list(void) ;

void draw_background(void);
errno_t sveska_load_font_list(sveska_t *sveska) ;
errno_t create_menu_bar(sveska_t *sveska);
void gfx_blend_pixel(gfx_context_t *gc, int x, int y, gfx_color_t *fg, gfx_color_t *bg, uint8_t alpha) ;

extern void wnd_pos_event(ui_window_t *window, void *arg, pos_event_t *event);
extern void cursor_setvis(bool visible);
extern int get_text_index_at_position(sveska_t *sveska, gfx_coord2_t mouse_pos);
extern void highlight_selected_text(sveska_t *sveska);
extern void sveska_get_position_from_index(sveska_t *sveska, int index, int *out_x, int *out_y);

extern void sveska_set_font(sveska_t *sveska, const char *font_path, float size);
extern void set_font_selected(sveska_t *sveska, const char *font_path, float size);
extern sveska_font_t* sveska_font_ref(sveska_font_t* font);
extern void sveska_font_unref(sveska_t *sveska, sveska_font_t *font);
extern void sveska_toggle_bold(sveska_t *sveska);
extern void sveska_toggle_italic(sveska_t *sveska);
extern void sveska_toggle_underline(sveska_t *sveska);

extern void dump_edit_history(sveska_t *sveska);
extern void debug_print_selection(sveska_t *sveska, int start, int end);
extern uint64_t get_timestamp(void);


extern void rebuild_display_text(sveska_t *sveska);

extern bool split_span_at(sveska_t *sveska, size_t span_idx, size_t split_pos);
 // sveska_font.c
extern void get_span_and_pos(sveska_t *sveska, size_t global_pos, 
    size_t *out_span_idx, size_t *out_span_pos);
 // sveska_font.c
extern bool sveska_config_load(sveska_t *sveska, const char *path);
    // main.c
extern void handle_keypress(sveska_t *sveska, const kbd_event_t *event);

extern void merge_with_previous_span(sveska_t *sveska);
extern bool split_current_span(sveska_t *sveska); 
extern bool should_split_span(sveska_t *sveska, char new_char);
extern void update_cursor_position(sveska_t *sveska);

//sveska_edit.c
extern void record_edit(sveska_t *sveska, edit_operation_t op);
extern void delete_selected_text(sveska_t *sveska);
extern void toggle_bold_selected(sveska_t *sveska);
extern void toggle_italic_selected(sveska_t *sveska);
extern void toggle_underline_selected(sveska_t *sveska);

extern int get_document_length(sveska_t *sveska);
/* Helper to check if there's an active selection */
extern bool has_selection(sveska_t *sveska);
extern void rebuild_font_metrics(sveska_t *sveska);
extern text_span_t *get_current_span(sveska_t *sveska);
extern bool create_new_span(sveska_t *sveska);
extern bool needs_new_span(sveska_t *sveska, text_span_t *span);
extern void handle_newline(sveska_t *sveska);
extern int calculate_current_line_height(sveska_t *sveska);
extern void handle_backspace(sveska_t *sveska);
//sveska_util.c
extern bool initialize_document(sveska_t *sveska);
extern void trim_whitespace(char *str);

//sveska_combo.c
extern bool is_special_combo(const kbd_event_t *event);
extern void handle_regular_key(sveska_t *sveska, const kbd_event_t *event);
extern void handle_special_combo(sveska_t *sveska, const kbd_event_t *event);
extern void handle_keyboard_event(sveska_t *sveska, const kbd_event_t *event);
extern void handle_copy(sveska_t *sveska);
extern void handle_paste(sveska_t *sveska);
extern void handle_cut(sveska_t *sveska);

// crtamo char_cursor
extern void draw_char_cursor(sveska_t *sveska);

/*  IstorijA***********/
//  sveska_text.c
extern void init_history(sveska_t *sveska);
// sveska_combo.c
extern void perform_undo(sveska_t *sveska);
extern void perform_redo(sveska_t *sveska);
// sveska_edit.c
extern void apply_operation(sveska_t *sveska, edit_operation_t *op, bool undo);




extern errno_t clipboard_put_rich_text(sveska_t *sveska, clipboard_data_t *data);
extern clipboard_data_t *clipboard_get_rich_text(sveska_t *sveska);
extern void clipboard_free_data(clipboard_data_t *data);
extern void get_char_style(text_span_t *span, size_t pos_in_span,
    bool *bold, bool *italic, bool *underline);
extern void ensure_style_arrays(text_span_t *span);
extern void free_span(text_span_t *span);
extern char *serialize_styles(clipboard_data_t *data);
extern clipboard_data_t *deserialize_styles(const char *buffer);
extern int compute_underline_y(sveska_font_t *font, float scale, int baseline_y);


extern int calculate_empty_line_height(sveska_t *sveska);
extern bool is_line_empty(sveska_t *sveska, size_t cursor_index);

extern size_t get_line_count(sveska_t *sveska);
extern int get_line_y(sveska_t *sveska, size_t line_num);
extern void print_line_y_positions(sveska_t *sveska);

extern void sveska_init_utf8(sveska_t *sveska);
extern size_t utf8_char_length(const char *str);
extern utf8_char_t get_utf8_char(const char *str, size_t pos);
extern int utf8_strlen(const char *str);

extern bool is_serbian_key(int key);
extern utf8_char_t get_serbian_utf8_char(const kbd_event_t *event);
extern utf8_char_t translate_key_to_char_serbian_cyrillic(const kbd_event_t *event);
extern char translate_key_to_char(const kbd_event_t *event);
extern void insert_text_with_style(sveska_t *sveska, const char *text, size_t length,
    const bool *bold, const bool *italic, const bool *underline,
    const char *font_path, float font_size);

/* Navigation */
extern int get_current_line(sveska_t *sveska);
extern int get_line_start(sveska_t *sveska, int line);
extern int get_line_end(sveska_t *sveska, int line);
extern int find_vertical_position(sveska_t *sveska, int target_line, int target_x);
extern int get_line_count_keyboard(sveska_t *sveska);

/* Cursor movement */
extern void move_cursor_left(sveska_t *sveska);
extern void move_cursor_right(sveska_t *sveska);
extern void move_cursor_up(sveska_t *sveska);
extern void move_cursor_down(sveska_t *sveska);
extern void move_cursor_home(sveska_t *sveska, bool extend_selection);
extern void move_cursor_end(sveska_t *sveska, bool extend_selection);

/* Text insertion */
extern void insert_char_with_current_style(sveska_t *sveska, char ch);
extern void clear_selection(sveska_t *sveska);

extern bool save_document(sveska_t *sveska, const char *path);
extern bool load_document(sveska_t *sveska, const char *path);
extern bool open_txt_file(sveska_t *sveska, const char *path);
extern void insert_image_span(sveska_t *sveska, int width, int height);

extern void insert_image_placeholder(sveska_t *sveska);
extern bool create_image_bitmap(sveska_t *sveska, text_span_t *span);



#endif

/** @}
 */