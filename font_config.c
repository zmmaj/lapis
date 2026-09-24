

/** @addtogroup lapis
 * @{
 */
/** @file Aplikacija za pisanje texta
 */



#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <dirent.h>
#include <errno.h>
#include <str.h>
#include "sveska.h"



 bool is_font_file(const char *filename) {
    const char *ext = str_rchr(filename, '.');
    return ext && (str_cmp(ext, ".ttf") == 0 || str_cmp(ext, ".otf") == 0);
}

errno_t scan_and_save_font_list(void) {
    const char *font_dir = "/data/font/";
    const char *output_file = "/data/font_list.txt";
    
    DIR *dir = opendir(font_dir);
    if (!dir) return errno;

    // First pass: count font files
    struct dirent *ent;
    size_t count = 0;
    while ((ent = readdir(dir))) {
        if (is_font_file(ent->d_name)) count++;
    }
    rewinddir(dir);

    // Allocate array
    char **font_names = calloc(count, sizeof(char *));
    if (!font_names) {
        closedir(dir);
        return ENOMEM;
    }

    // Second pass: store names
    size_t idx = 0;
    while ((ent = readdir(dir)) && idx < count) {
        if (is_font_file(ent->d_name)) {
            font_names[idx] = str_dup(ent->d_name);
            if (!font_names[idx]) {
                closedir(dir);
                // Cleanup what we allocated so far
                for (size_t i = 0; i < idx; i++) free(font_names[i]);
                free(font_names);
                return ENOMEM;
            }
            idx++;
        }
    }
    closedir(dir);

    // Write to file
    FILE *f = fopen(output_file, "w");
    if (!f) {
        for (size_t i = 0; i < count; i++) free(font_names[i]);
        free(font_names);
        return errno;
    }

    for (size_t i = 0; i < count; i++) {
        fprintf(f, "%s\n", font_names[i]);
        free(font_names[i]); // Free each name as we go
    }
    free(font_names);
    fclose(f);

    return EOK;
}



errno_t sveska_load_font_list(sveska_t *sveska) {
    FILE *f = fopen("/data/font_list.txt", "rb");
    if (!f) {
        printf("Ne mogu da otvorim: /data/font_list.txt\n");
        return ENOENT;
    }

    sveska->font_count = 0;
    sveska->current_font_index = (size_t)-1;  // Initialize to invalid

    char line[MAX_FONT_NAME_LEN];
    while (fgets(line, sizeof(line), f)) {
        size_t len = str_length(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (sveska->font_count >= MAX_FONTS) break;

        char fullpath[256];
        snprintf(fullpath, sizeof(fullpath), "/data/font/%s", line);

        FILE *fttf = fopen(fullpath, "rb");
        if (!fttf) {
            printf("Could not open font file: %s\n", fullpath);
            continue;
        }

        fseek(fttf, 0, SEEK_END);
        size_t size = ftell(fttf);
        fseek(fttf, 0, SEEK_SET);

        unsigned char *buffer = malloc(size);
        if (!buffer) {
            fclose(fttf);
            printf("Alokacija memorije neuspela za font: %s\n", line);
            continue;
        }

        fread(buffer, 1, size, fttf);
        fclose(fttf);

        stbtt_fontinfo finfo;
        if (!stbtt_InitFont(&finfo, buffer, 0)) {
            printf("Neuspelo iniciranje TTF fonta: %s\n", line);
            free(buffer);
            continue;
        }

        int ascent, descent, linegap;
        stbtt_GetFontVMetrics(&finfo, &ascent, &descent, &linegap);

        if (ascent <= 0 || descent >= 0) {
            printf("Meispravna vertikalna metrika fonta: %s (ascent=%d, descent=%d) — preskacem ga\n",
                   line, ascent, descent);
            free(buffer);
            continue;
        }

        if (linegap == 0) {
            linegap = ascent / 5;
           // printf("Font %s has linegap=0, assigning fallback linegap=%d\n", line, linegap);
        }

        sveska_font_t *dst = &sveska->fonts[sveska->font_count];
        dst->is_dynamic = false;
        str_cpy(dst->name, MAX_FONT_NAME_LEN, line);
        str_cpy(dst->path, sizeof(dst->path), fullpath);
        str_cpy(sveska->font_names[sveska->font_count], MAX_FONT_NAME_LEN, line);

        dst->font_data = buffer;
        dst->ttf_size = size;
        dst->info = finfo;
        dst->ref_count = 1;

        float default_pixel_height = sveska->font_size;
        dst->size = default_pixel_height;
        dst->scale = stbtt_ScaleForPixelHeight(&dst->info, dst->size);

        dst->ascent = ascent;
        dst->descent = descent;
        dst->linegap = linegap;
        dst->baseline = (int)(dst->ascent * dst->scale);
        dst->line_height = (int)((dst->ascent - dst->descent + dst->linegap) * dst->scale);

        stbtt_GetFontBoundingBox(&dst->info, &dst->bbox_x0, &dst->bbox_y0, &dst->bbox_x1, &dst->bbox_y1);

        int space_advance, space_lsb;
        stbtt_GetCodepointHMetrics(&dst->info, ' ', &space_advance, &space_lsb);
        dst->space_width = (int)(space_advance * dst->scale);
        if (dst->space_width == 0) {
            dst->space_width = (int)(dst->scale * 5);
        }

        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBox(&dst->info, 'X', dst->scale, dst->scale, &x0, &y0, &x1, &y1);
        dst->char_width = x1 - x0;

        // Check estimated height and auto-scale up if needed
        float est_char_height = (float)(ascent - descent) * dst->scale;
        const float min_char_height = 20.0f;

        if (est_char_height < min_char_height) {
            float scale_factor = min_char_height / est_char_height;
            float new_size = dst->size * scale_factor;


            dst->size = new_size;
            dst->scale = stbtt_ScaleForPixelHeight(&dst->info, dst->size);

            // Recalculate metrics
            dst->baseline = (int)(dst->ascent * dst->scale);
            dst->line_height = (int)((dst->ascent - dst->descent + dst->linegap) * dst->scale);
            dst->space_width = (int)(space_advance * dst->scale);

            stbtt_GetCodepointBitmapBox(&dst->info, 'X', dst->scale, dst->scale, &x0, &y0, &x1, &y1);
            dst->char_width = x1 - x0;
        }

        if (sveska->current_font_index == (size_t)-1 &&
            str_casecmp(line, "arial.ttf") == 0) {
            sveska->current_font_index = sveska->font_count;

        }

        sveska->font_count++;
    }

    fclose(f);

    if (sveska->current_font_index == (size_t)-1 && sveska->font_count > 0) {
        sveska->current_font_index = 0;
    }

    return EOK;
}





/** @}
 */


