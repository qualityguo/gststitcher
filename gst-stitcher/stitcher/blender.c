#include "blender.h"
#include <string.h>
#include <glib.h>

OverlapInfo *blender_detect_overlaps(const CanvasInfo *canvas)
{
    OverlapInfo *info = g_new0(OverlapInfo, 1);

    int count = 0;
    for (int i = 0; i < canvas->num_images && count < STITCHER_MAX_INPUTS; i++) {
        for (int j = i + 1; j < canvas->num_images && count < STITCHER_MAX_INPUTS; j++) {
            const CanvasInfo *ci = canvas;
            int ax = ci->images[i].warp_x;
            int ay = ci->images[i].warp_y;
            int aw = ci->images[i].warp_w;
            int ah = ci->images[i].warp_h;

            int bx = ci->images[j].warp_x;
            int by = ci->images[j].warp_y;
            int bw = ci->images[j].warp_w;
            int bh = ci->images[j].warp_h;

            int ox = MAX(ax, bx);
            int oy = MAX(ay, by);
            int ow = MIN(ax + aw, bx + bw) - ox;
            int oh = MIN(ay + ah, by + bh) - oy;

            if (ow > 0 && oh > 0) {
                info->regions[count].x = ox;
                info->regions[count].y = oy;
                info->regions[count].w = ow;
                info->regions[count].h = oh;
                info->pair_indices[count] = i; /* store first image index */
                count++;
            }
        }
    }

    info->num_regions = count;
    return info;
}

void blender_free_overlaps(OverlapInfo *info)
{
    if (info)
        g_free(info);
}

uint8_t *blender_save_region(const uint8_t *canvas, int canvas_stride,
    const StitcherRect *region)
{
    int size = region->h * region->w * 4;
    uint8_t *saved = g_malloc(size);
    if (!saved)
        return NULL;

    for (int y = 0; y < region->h; y++) {
        const uint8_t *src = canvas + (region->y + y) * canvas_stride + region->x * 4;
        uint8_t *dst = saved + y * region->w * 4;
        memcpy(dst, src, region->w * 4);
    }

    return saved;
}

void blender_apply(const StitcherBackend *backend, void *ctx,
    uint8_t *canvas, int canvas_stride,
    const uint8_t *saved, int saved_stride,
    const StitcherRect *region,
    int border_width)
{
    if (!backend || !backend->blend)
        return;

    /* saved buffer starts at (0,0) of the overlap region, not canvas coordinates */
    backend->blend(ctx, canvas, canvas_stride,
        region->x, region->y, region->w, region->h,
        saved, saved_stride,
        0, 0,
        border_width);
}

void blender_free_saved(uint8_t *saved)
{
    if (saved)
        g_free(saved);
}
