#ifndef WARP_BACKEND_H
#define WARP_BACKEND_H

#include "stitcher_types.h"

typedef struct _StitcherBackend StitcherBackend;

typedef void* (*StitcherBackendInitFunc)(int canvas_w, int canvas_h);
typedef void  (*StitcherBackendWarpFunc)(void *ctx,
    const uint8_t *src, int src_w, int src_h, int src_stride,
    uint8_t *dst, int dst_stride,
    int dst_offset_x, int dst_offset_y,
    int warp_w, int warp_h,
    const float h_inv[3][3],
    int crop_left, int crop_top);
typedef void  (*StitcherBackendBlendFunc)(void *ctx,
    uint8_t *canvas, int canvas_stride,
    int region_x, int region_y, int region_w, int region_h,
    const uint8_t *overlay, int overlay_stride,
    int overlay_x, int overlay_y,
    int border_width);
typedef void  (*StitcherBackendCleanupFunc)(void *ctx);

struct _StitcherBackend {
    StitcherBackendInitFunc    init;
    StitcherBackendWarpFunc    warp;
    StitcherBackendBlendFunc   blend;
    StitcherBackendCleanupFunc cleanup;
};

const StitcherBackend *stitcher_backend_cpu_get(void);

/* Map-based backend API v2 */
void map_backend_set_map_data_v2(void *ctx, const MapFileDataV2 *map_data);
void map_backend_set_image_index(void *ctx, int image_index);
const StitcherBackend *stitcher_backend_map_get(void);

#endif /* WARP_BACKEND_H */
