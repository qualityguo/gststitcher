#include "warp_backend.h"
#include <math.h>
#include <stddef.h>

static void *cpu_backend_init(int canvas_w, int canvas_h)
{
    /* CPU backend needs no special context */
    return NULL;
}

static void cpu_backend_warp(void *ctx,
    const uint8_t *src, int src_w, int src_h, int src_stride,
    uint8_t *dst, int dst_stride,
    int dst_offset_x, int dst_offset_y,
    int warp_w, int warp_h,
    const float h_inv[3][3],
    int crop_left, int crop_top)
{
    (void)ctx;

    for (int oy = 0; oy < warp_h; oy++) {
        uint8_t *dst_row = dst + (dst_offset_y + oy) * dst_stride + dst_offset_x * 4;

        for (int ox = 0; ox < warp_w; ox++) {
            /* Use canvas coordinates for homography mapping */
            float cx = (float)(dst_offset_x + ox);
            float cy = (float)(dst_offset_y + oy);

            /* h_inv maps canvas coordinates → source coordinates */
            float w = h_inv[2][0] * cx + h_inv[2][1] * cy + h_inv[2][2];
            float src_x = (h_inv[0][0] * cx + h_inv[0][1] * cy + h_inv[0][2]) / w + crop_left;
            float src_y = (h_inv[1][0] * cx + h_inv[1][1] * cy + h_inv[1][2]) / w + crop_top;

            /* Bilinear interpolation */
            int x0 = (int)floorf(src_x);
            int y0 = (int)floorf(src_y);
            int x1 = x0 + 1;
            int y1 = y0 + 1;
            float fx = src_x - (float)x0;
            float fy = src_y - (float)y0;

            uint8_t *out = dst_row + ox * 4;

            /* Handle edge pixels: clamp to valid range */
            if (x0 < 0 || x0 >= src_w || y0 < 0 || y0 >= src_h) {
                out[0] = 0; out[1] = 0; out[2] = 0; out[3] = 0;
                continue;
            }

            /* For edge pixels where x1 or y1 is out of bounds, use available pixels */
            if (x1 >= src_w) x1 = x0;
            if (y1 >= src_h) y1 = y0;

            const uint8_t *p00 = src + y0 * src_stride + x0 * 4;
            const uint8_t *p10 = src + y0 * src_stride + x1 * 4;
            const uint8_t *p01 = src + y1 * src_stride + x0 * 4;
            const uint8_t *p11 = src + y1 * src_stride + x1 * 4;

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

static void cpu_backend_cleanup(void *ctx)
{
    (void)ctx;
}

static const StitcherBackend cpu_backend = {
    .init    = cpu_backend_init,
    .warp    = cpu_backend_warp,
    .blend   = NULL,  /* set in blend_cpu.c */
    .cleanup = cpu_backend_cleanup,
};

/* Blend function defined in blend_cpu.c */
extern void cpu_blend_func(void *ctx,
    uint8_t *canvas, int canvas_stride,
    int region_x, int region_y, int region_w, int region_h,
    const uint8_t *overlay, int overlay_stride,
    int overlay_x, int overlay_y,
    int border_width);

static StitcherBackend cpu_backend_instance;

const StitcherBackend *stitcher_backend_cpu_get(void)
{
    /* Copy template and plug in blend function */
    cpu_backend_instance = cpu_backend;
    cpu_backend_instance.blend = cpu_blend_func;
    return &cpu_backend_instance;
}
