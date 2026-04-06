#include "map_generator.h"
#include "../stitcher/stitcher_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <glib.h>
#include "../stitcher/homography_config.h"
#include "../stitcher/canvas.h"

#define MAP_VERSION 2
#define MAX_ERROR_LEN 512
#define MAX_PATH_LEN 512

/* Error handling helper */
static void set_error(char **error_msg, const char *format, ...)
{
    if (error_msg == NULL)
        return;

    va_list args;
    *error_msg = (char *)malloc(MAX_ERROR_LEN);
    if (*error_msg) {
        va_start(args, format);
        vsnprintf(*error_msg, MAX_ERROR_LEN, format, args);
        va_end(args);
    }
}

/* Create directory if it doesn't exist */
static int ensure_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode) ? 0 : -1;
    }
    return mkdir(path, 0755);
}

/* Generate validated coordinate map for a single image with CPU-matching boundary handling */
static ValidatedCoordinate *generate_single_map_v2(
    const CanvasInfo *canvas_info,
    const StitcherConfig *config,
    int image_index,
    const ImageSize *input_size,
    const StitcherCrop *crop)
{
    /* Generate map only for the warp region of this image */
    int warp_x = canvas_info->images[image_index].warp_x;
    int warp_y = canvas_info->images[image_index].warp_y;
    int map_w = canvas_info->images[image_index].warp_w;
    int map_h = canvas_info->images[image_index].warp_h;

    fprintf(stderr, "Generating map v2 for image %d: warp_region=%d,%d,%d,%d, offset=(%d,%d)\n",
            image_index, warp_x, warp_y, map_w, map_h,
            canvas_info->images[image_index].offset_x,
            canvas_info->images[image_index].offset_y);

    /* Allocate validated coordinate map */
    ValidatedCoordinate *map = (ValidatedCoordinate *)malloc(map_w * map_h * sizeof(ValidatedCoordinate));
    if (map == NULL)
        return NULL;

    /* Get inverse homography for this image */
    float h_inv[3][3];

    /* Reference image uses identity matrix */
    if (image_index == config->reference_index) {
        /* Identity matrix */
        h_inv[0][0] = 1.0f; h_inv[0][1] = 0.0f; h_inv[0][2] = 0.0f;
        h_inv[1][0] = 0.0f; h_inv[1][1] = 1.0f; h_inv[1][2] = 0.0f;
        h_inv[2][0] = 0.0f; h_inv[2][1] = 0.0f; h_inv[2][2] = 1.0f;
    } else {
        /* Find the pair config for this image */
        const StitcherPairConfig *pair = NULL;
        for (int i = 0; i < config->num_inputs; i++) {
            if (config->pairs[i].target == image_index) {
                pair = &config->pairs[i];
                break;
            }
        }

        if (pair == NULL) {
            free(map);
            return NULL;
        }

        memcpy(h_inv, pair->homography_inv.h, 9 * sizeof(float));
    }

    /* Get source image dimensions */
    int src_w = input_size->w;
    int src_h = input_size->h;

    /* Get canvas offset from canvas_info.
     * The offset is the amount needed to shift the reference frame so that
     * its origin aligns with canvas (0, 0).
     */
    float offset_x = (float)canvas_info->images[config->reference_index].offset_x;
    float offset_y = (float)canvas_info->images[config->reference_index].offset_y;

    /* Generate coordinates for each pixel in the warp region */
    static int image_dbg_count = 0;  /* Track which images we've debugged */
    int should_debug = (image_dbg_count < 3);  /* Debug first 3 images */

    for (int oy = 0; oy < map_h; oy++) {
        for (int ox = 0; ox < map_w; ox++) {
            /* Map warp region coordinates to canvas coordinates */
            int canvas_x = warp_x + ox;
            int canvas_y = warp_y + oy;

            /* Apply canvas offset to get coordinates in the reference image's coordinate system */
            float cx = (float)canvas_x - offset_x;
            float cy = (float)canvas_y - offset_y;

            float w = h_inv[2][0] * cx + h_inv[2][1] * cy + h_inv[2][2];

            float src_x, src_y;
            uint8_t flags = 0;

            /* Protect against division by zero or very small w */
            if (fabsf(w) < 1e-10f) {
                /* Mark as invalid */
                flags &= ~COORD_FLAG_IS_VALID;
                src_x = src_y = 0.0f;
            } else {
                /* Compute source coordinates */
                src_x = (h_inv[0][0] * cx + h_inv[0][1] * cy + h_inv[0][2]) / w;
                src_y = (h_inv[1][0] * cx + h_inv[1][1] * cy + h_inv[1][2]) / w;

                /* Apply crop offset to match CPU backend behavior */
                if (crop) {
                    src_x += crop->left;
                    src_y += crop->top;
                }

                /* Compute integer coordinates */
                int x0 = (int)floorf(src_x);
                int y0 = (int)floorf(src_y);
                int x1 = x0 + 1;
                int y1 = y0 + 1;

                /* CPU-MATCHING BOUNDARY HANDLING */
                /* This is the KEY FIX for the "花边" artifact */

                /* Check if primary coordinates are in bounds */
                if (x0 < 0 || x0 >= src_w || y0 < 0 || y0 >= src_h) {
                    /* Primary coordinates out of bounds - mark as invalid */
                    flags &= ~COORD_FLAG_IS_VALID;
                    /* Debug: print first few invalid coordinates */
                    if (should_debug && ox == 0 && oy < 5) {
                        fprintf(stderr, "Map gen img %d: INVALID coord at canvas=(%d,%d): src_x=%.2f, src_y=%.2f, x0=%d, y0=%d, src_w=%d, src_h=%d\n",
                                image_index, canvas_x, canvas_y, src_x, src_y, x0, y0, src_w, src_h);
                    }
                    /* Clamp to valid range for storage */
                    x0 = (x0 < 0) ? 0 : (x0 >= src_w ? src_w - 1 : x0);
                    y0 = (y0 < 0) ? 0 : (y0 >= src_h ? src_h - 1 : y0);
                } else {
                    /* Primary coordinates valid */
                    flags |= COORD_FLAG_IS_VALID;
                    /* Debug: print first few valid coordinates */
                    if (should_debug && ox == 0 && oy < 5) {
                        fprintf(stderr, "Map gen img %d: VALID coord at canvas=(%d,%d): src_x=%.2f, src_y=%.2f, x0=%d, y0=%d\n",
                                image_index, canvas_x, canvas_y, src_x, src_y, x0, y0);
                    }

                    /* CPU backend behavior: check secondary coordinates */
                    /* If x1 is out of bounds, use x0 (NOT src_w-1) */
                    if (x1 >= src_w) {
                        x1 = x0;  /* CPU behavior: use same pixel */
                        flags &= ~COORD_FLAG_VALID_X1;  /* Indicate edge case */
                    } else {
                        flags |= COORD_FLAG_VALID_X1;  /* Normal case */
                    }

                    /* If y1 is out of bounds, use y0 (NOT src_h-1) */
                    if (y1 >= src_h) {
                        y1 = y0;  /* CPU behavior: use same pixel */
                        flags &= ~COORD_FLAG_VALID_Y1;  /* Indicate edge case */
                    } else {
                        flags |= COORD_FLAG_VALID_Y1;  /* Normal case */
                    }
                }

                /* Store clamped coordinates */
                src_x = (float)x0;
                src_y = (float)y0;
            }

            /* Quantize and store coordinate */
            int idx = oy * map_w + ox;
            map[idx].x0 = (uint16_t)((int)floorf(src_x));
            map[idx].y0 = (uint16_t)((int)floorf(src_y));
            map[idx].fx = (uint8_t)(fmodf(src_x, 1.0f) * 255.0f);
            map[idx].fy = (uint8_t)(fmodf(src_y, 1.0f) * 255.0f);
            map[idx].flags = flags;
            map[idx].reserved = 0;
        }
    }

    fprintf(stderr, "Generated %d coordinates for image %d\n", map_w * map_h, image_index);

    image_dbg_count++;  /* Mark this image as debugged */

    return map;
}

/* Write JSON metadata file */
static bool write_metadata_json(
    const char *output_dir,
    const CanvasInfo *canvas_info,
    const ImageSize *input_sizes,
    const StitcherCrop *crops,
    int num_inputs,
    char **error_msg)
{
    char metadata_path[MAX_PATH_LEN];
    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.json", output_dir);

    FILE *fp = fopen(metadata_path, "w");
    if (!fp) {
        set_error(error_msg, "Failed to create metadata.json: %s", metadata_path);
        return false;
    }

    /* Write JSON header */
    fprintf(fp, "{\n");
    fprintf(fp, "  \"magic\": \"GSTSTMAP\",\n");
    fprintf(fp, "  \"version\": %d,\n", MAP_VERSION);
    fprintf(fp, "  \"canvas\": {\n");
    fprintf(fp, "    \"width\": %d,\n", canvas_info->canvas_w);
    fprintf(fp, "    \"height\": %d\n", canvas_info->canvas_h);
    fprintf(fp, "  },\n");
    fprintf(fp, "  \"inputs\": [\n");

    /* Write each input */
    for (int i = 0; i < num_inputs; i++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"index\": %d,\n", i);
        fprintf(fp, "      \"width\": %d,\n", input_sizes[i].w);
        fprintf(fp, "      \"height\": %d,\n", input_sizes[i].h);
        fprintf(fp, "      \"crop\": {\n");
        fprintf(fp, "        \"left\": %d,\n", crops ? crops[i].left : 0);
        fprintf(fp, "        \"right\": %d,\n", crops ? crops[i].right : 0);
        fprintf(fp, "        \"top\": %d,\n", crops ? crops[i].top : 0);
        fprintf(fp, "        \"bottom\": %d\n", crops ? crops[i].bottom : 0);
        fprintf(fp, "      },\n");
        fprintf(fp, "      \"warp_region\": {\n");
        fprintf(fp, "        \"x\": %d,\n", canvas_info->images[i].warp_x);
        fprintf(fp, "        \"y\": %d,\n", canvas_info->images[i].warp_y);
        fprintf(fp, "        \"width\": %d,\n", canvas_info->images[i].warp_w);
        fprintf(fp, "        \"height\": %d\n", canvas_info->images[i].warp_h);
        fprintf(fp, "      },\n");
        fprintf(fp, "      \"coordinate_data\": \"coords_%d.bin\"\n", i);
        fprintf(fp, "    }%s\n", i < num_inputs - 1 ? "," : "");
    }

    fprintf(fp, "  ],\n");
    fprintf(fp, "  \"metadata\": {\n");
    fprintf(fp, "    \"generation_timestamp\": \"2026-04-05T12:00:00Z\",\n");
    fprintf(fp, "    \"generator_version\": \"2.0\",\n");
    fprintf(fp, "    \"boundary_method\": \"cpu_match\"\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "}\n");

    fclose(fp);
    fprintf(stderr, "Wrote metadata.json to %s\n", metadata_path);

    return true;
}

bool generate_map_file_v2(
    const char *homography_json,
    const char *output_dir,
    const ImageSize *input_sizes,
    int num_inputs,
    const StitcherCrop *crops,
    char **error_msg)
{
    StitcherConfig *config = NULL;
    CanvasInfo *canvas_info = NULL;
    ValidatedCoordinate **mappings = NULL;
    bool success = false;
    GError *gerror = NULL;

    if (error_msg)
        *error_msg = NULL;

    fprintf(stderr, "=== Map Generator v2 ===\n");
    fprintf(stderr, "Homography: %s\n", homography_json);
    fprintf(stderr, "Output: %s\n", output_dir);
    fprintf(stderr, "Inputs: %d\n", num_inputs);

    /* Create output directory */
    if (ensure_directory(output_dir) != 0) {
        set_error(error_msg, "Failed to create output directory: %s", output_dir);
        goto cleanup;
    }

    /* Load homography configuration */
    config = stitcher_config_parse_file(homography_json, &gerror);
    if (config == NULL) {
        set_error(error_msg, "Failed to parse homography file: %s",
                  gerror ? gerror->message : "unknown");
        if (gerror)
            g_error_free(gerror);
        goto cleanup;
    }

    /* Validate input count */
    if (config->num_inputs != num_inputs) {
        set_error(error_msg, "Input count mismatch: config has %d, provided %d",
                  config->num_inputs, num_inputs);
        goto cleanup;
    }

    /* Compute canvas geometry */
    canvas_info = canvas_compute(config, input_sizes, crops);
    if (canvas_info == NULL) {
        set_error(error_msg, "Failed to compute canvas geometry");
        goto cleanup;
    }

    fprintf(stderr, "Canvas: %dx%d\n", canvas_info->canvas_w, canvas_info->canvas_h);

    /* Allocate mappings array */
    mappings = (ValidatedCoordinate **)calloc(num_inputs, sizeof(ValidatedCoordinate *));
    if (mappings == NULL) {
        set_error(error_msg, "Memory allocation failed");
        goto cleanup;
    }

    /* Generate coordinate maps for each input */
    for (int i = 0; i < num_inputs; i++) {
        mappings[i] = generate_single_map_v2(canvas_info, config, i,
                                              &input_sizes[i],
                                              crops ? &crops[i] : NULL);
        if (mappings[i] == NULL) {
            set_error(error_msg, "Failed to generate map for image %d", i);
            goto cleanup;
        }
    }

    /* Write JSON metadata */
    if (!write_metadata_json(output_dir, canvas_info, input_sizes, crops, num_inputs, error_msg)) {
        goto cleanup;
    }

    /* Write binary coordinate files */
    for (int i = 0; i < num_inputs; i++) {
        char coord_path[MAX_PATH_LEN];
        snprintf(coord_path, sizeof(coord_path), "%s/coords_%d.bin", output_dir, i);

        FILE *fp = fopen(coord_path, "wb");
        if (!fp) {
            set_error(error_msg, "Failed to create coordinate file: %s", coord_path);
            goto cleanup;
        }

        int warp_w = canvas_info->images[i].warp_w;
        int warp_h = canvas_info->images[i].warp_h;
        size_t data_size = (size_t)warp_w * (size_t)warp_h * sizeof(ValidatedCoordinate);

        fprintf(stderr, "Writing coords_%d.bin: %zu bytes (%dx%d)\n", i, data_size, warp_w, warp_h);

        size_t written = fwrite(mappings[i], 1, data_size, fp);
        fclose(fp);

        if (written != data_size) {
            set_error(error_msg, "Failed to write coordinate file for image %d: wrote %zu of %zu bytes",
                     i, written, data_size);
            goto cleanup;
        }
    }

    fprintf(stderr, "=== Map generation complete ===\n");
    success = true;

cleanup:
    if (mappings) {
        for (int i = 0; i < num_inputs; i++) {
            free(mappings[i]);
        }
        free(mappings);
    }
    if (canvas_info)
        canvas_free(canvas_info);
    if (config)
        stitcher_config_free(config);

    return success;
}
