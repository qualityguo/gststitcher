#ifndef BLENDER_H
#define BLENDER_H

#include "stitcher_types.h"
#include "warp_backend.h"

OverlapInfo *blender_detect_overlaps(const CanvasInfo *canvas);
void         blender_free_overlaps(OverlapInfo *info);

/*
 * Save overlap region from canvas before it gets overwritten.
 * Returns a newly allocated buffer containing the saved pixels.
 */
uint8_t *blender_save_region(const uint8_t *canvas, int canvas_stride,
    const StitcherRect *region);

/*
 * Apply blending: mix saved region with current canvas content.
 */
void blender_apply(const StitcherBackend *backend, void *ctx,
    uint8_t *canvas, int canvas_stride,
    const uint8_t *saved, int saved_stride,
    const StitcherRect *region,
    int border_width);

void blender_free_saved(uint8_t *saved);

#endif /* BLENDER_H */
