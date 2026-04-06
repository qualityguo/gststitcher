#include "warp_backend.h"
#include "map_loader.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * Map-based warp backend v2
 *
 * Uses pre-computed coordinate mapping with CPU-matching boundary handling.
 * Eliminates "花边" (edge artifacts) by using same boundary logic as CPU backend.
 */

typedef struct {
    const MapFileDataV2 *map_data;
    int canvas_w;
    int canvas_h;
    int current_image_index;  /* Which image to warp */
} MapBackendContext;

static void *map_backend_init(int canvas_w, int canvas_h)
{
    // fprintf(stderr, "map_backend_init v2: canvas_w=%d, canvas_h=%d\n", canvas_w, canvas_h);
    MapBackendContext *ctx = (MapBackendContext *)calloc(1, sizeof(MapBackendContext));
    if (ctx == NULL)
        return NULL;

    ctx->canvas_w = canvas_w;
    ctx->canvas_h = canvas_h;
    ctx->current_image_index = 0;  /* Default to first image */

    // fprintf(stderr, "map_backend_init v2: initialized ctx=%p\n", ctx);
    return ctx;
}

void map_backend_set_map_data_v2(void *ctx, const MapFileDataV2 *map_data)
{
    // fprintf(stderr, "map_backend_set_map_data_v2: ctx=%p, map_data=%p\n", ctx, map_data);
    MapBackendContext *map_ctx = (MapBackendContext *)ctx;
    if (map_ctx) {
        map_ctx->map_data = map_data;
        // fprintf(stderr, "map_backend_set_map_data_v2: successfully set map_data (v2)\n");
    } else {
        fprintf(stderr, "map_backend_set_map_data_v2: ERROR - ctx is NULL!\n");
    }
}

void map_backend_set_image_index(void *ctx, int image_index)
{
    MapBackendContext *map_ctx = (MapBackendContext *)ctx;
    if (map_ctx) {
        map_ctx->current_image_index = image_index;
    }
}

static void map_backend_warp(void *ctx,
    const uint8_t *src, int src_w, int src_h, int src_stride,
    uint8_t *dst, int dst_stride,
    int dst_offset_x, int dst_offset_y,
    int warp_w, int warp_h,
    const float h_inv[3][3],  /* IGNORED - using map instead */
    int crop_left, int crop_top)
{
    MapBackendContext *map_ctx = (MapBackendContext *)ctx;
    const MapFileDataV2 *map_data;
    int image_index;

    (void)h_inv;  /* Unused in map mode */
    (void)crop_left;  /* Already applied during map generation */
    (void)crop_top;

    // fprintf(stderr, "map_backend_warp: START ctx=%p, src=%p, dst=%p\n", ctx, src, dst);
    // fflush(stderr);

    if (map_ctx == NULL) {
        fprintf(stderr, "map_backend_warp: ERROR - map_ctx is NULL\n");
        return;
    }
    if (map_ctx->map_data == NULL) {
        fprintf(stderr, "map_backend_warp: ERROR - map_ctx->map_data is NULL\n");
        return;
    }

    map_data = map_ctx->map_data;
    image_index = map_ctx->current_image_index;

    // fprintf(stderr, "map_backend_warp: image_index=%d, num_inputs=%d\n", image_index, map_data->num_inputs);
    // fflush(stderr);

    if (map_data->num_inputs == 0 || image_index >= map_data->num_inputs) {
        fprintf(stderr, "map_backend_warp: ERROR - invalid image_index\n");
        return;
    }

    /* Get warp region for this image from map file */
    int warp_x = map_data->warp_x[image_index];
    int warp_y = map_data->warp_y[image_index];
    int map_w = map_data->warp_w[image_index];
    int map_h = map_data->warp_h[image_index];
    const ValidatedCoordinate *coords = map_data->coords[image_index];

    /* Validate coords array */
    if (!coords) {
        fprintf(stderr, "map_backend_warp_v2: ERROR - coords array is NULL for image %d\n", image_index);
        return;
    }

    /* Validate dimensions */
    if (warp_w <= 0 || warp_h <= 0 || map_w <= 0 || map_h <= 0) {
        fprintf(stderr, "map_backend_warp_v2: Invalid dimensions warp_w=%d warp_h=%d map_w=%d map_h=%d\n",
                warp_w, warp_h, map_w, map_h);
        return;
    }

    // fprintf(stderr, "map_backend_warp_v2: image=%d warp_region(canvas)=%d,%d,%d,%d output_size=%d,%d dst_pos(canvas)=%d,%d src=%dx%d\n",
    //         image_index, warp_x, warp_y, map_w, map_h, warp_w, warp_h,
    //         dst_offset_x, dst_offset_y, src_w, src_h);

    /* Process each output pixel */
    for (int oy = 0; oy < warp_h; oy++) {
        /* Calculate absolute canvas Y coordinate */
        int canvas_y = dst_offset_y + oy;

        /* Skip if outside warp region (shouldn't happen if caller is correct) */
        if (canvas_y < warp_y || canvas_y >= warp_y + map_h)
            continue;

        /* Calculate destination row pointer */
        uint8_t *dst_row = dst + canvas_y * dst_stride + dst_offset_x * 4;

        /* Pre-calculate map Y offset */
        const int map_y_offset = (canvas_y - warp_y) * map_w;

        for (int ox = 0; ox < warp_w; ox++) {
            /* Calculate absolute canvas X coordinate */
            int canvas_x = dst_offset_x + ox;

            /* Skip if outside warp region (shouldn't happen if caller is correct) */
            if (canvas_x < warp_x || canvas_x >= warp_x + map_w) {
                /* Out of bounds - set to black */
                uint8_t *out = dst_row + ox * 4;
                out[0] = out[1] = out[2] = out[3] = 0;
                continue;
            }

            /* Calculate map index - this is ALWAYS within bounds due to checks above */
            const int map_idx = map_y_offset + (canvas_x - warp_x);

            /* Get validated coordinate from map */
            const ValidatedCoordinate *coord = &coords[map_idx];

            /* Check if coordinate is marked as valid */
            if (!(coord->flags & COORD_FLAG_IS_VALID)) {
                /* Marked as invalid during map generation - output black */
                uint8_t *out = dst_row + ox * 4;
                out[0] = out[1] = out[2] = out[3] = 0;
                continue;
            }

            /* Reconstruct coordinates from quantized format */
            int x0 = (int)coord->x0;
            int y0 = (int)coord->y0;

            /* Reconstruct fractional parts */
            float fx = coord->fx / 255.0f;
            float fy = coord->fy / 255.0f;

            /* Use flags to determine secondary coordinates - THIS IS THE KEY FIX */
            /* CPU backend behavior: x1 = x0 for edge pixels (when x1 >= src_w) */
            int x1 = (coord->flags & COORD_FLAG_VALID_X1) ? x0 + 1 : x0;
            int y1 = (coord->flags & COORD_FLAG_VALID_Y1) ? y0 + 1 : y0;

            /* No bounds checking needed here - validation done during map generation */
            /* All coordinates are guaranteed to be within source image bounds */

            /* Access source pixels - SAFE due to validation above */
            const uint8_t *p00 = src + y0 * src_stride + x0 * 4;
            const uint8_t *p10 = src + y0 * src_stride + x1 * 4;
            const uint8_t *p01 = src + y1 * src_stride + x0 * 4;
            const uint8_t *p11 = src + y1 * src_stride + x1 * 4;

            uint8_t *out = dst_row + ox * 4;

            /* Bilinear interpolation */
            for (int c = 0; c < 4; c++) {
                float v = (1.0f - fx) * (1.0f - fy) * p00[c]
                        + fx * (1.0f - fy) * p10[c]
                        + (1.0f - fx) * fy * p01[c]
                        + fx * fy * p11[c];
                int iv = (int)(v + 0.5f);
                out[c] = (uint8_t)(iv < 0 ? 0 : (iv > 255 ? 255 : iv));
            }
        }
    }
}

static void map_backend_cleanup(void *ctx)
{
    if (ctx) {
        MapBackendContext *map_ctx = (MapBackendContext *)ctx;
        /* Don't free map_data - it's owned by the element */
        free(map_ctx);
    }
}

/* Blend function is the same as CPU backend */
extern void cpu_blend_func(void *ctx,
    uint8_t *canvas, int canvas_stride,
    int region_x, int region_y, int region_w, int region_h,
    const uint8_t *overlay, int overlay_stride,
    int overlay_x, int overlay_y,
    int border_width);

const StitcherBackend *stitcher_backend_map_get(void)
{
    static StitcherBackend backend = {
        .init    = map_backend_init,
        .warp    = map_backend_warp,
        .blend   = cpu_blend_func,
        .cleanup = map_backend_cleanup
    };

    /* Return static instance */
    return &backend;
}
