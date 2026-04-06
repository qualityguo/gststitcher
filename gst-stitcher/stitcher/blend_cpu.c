#include <stdint.h>

/*
 * CPU linear blend: mix saved overlay (img0) with current canvas (img1)
 * in the overlap region using a horizontal gradient.
 *
 * canvas  = output buffer, currently holding img1's warped pixels
 * overlay = saved copy of img0's pixels in the overlap region (same coordinates)
 */
void cpu_blend_func(void *ctx,
    uint8_t *canvas, int canvas_stride,
    int region_x, int region_y, int region_w, int region_h,
    const uint8_t *overlay, int overlay_stride,
    int overlay_x, int overlay_y,
    int border_width)
{
    (void)ctx;

    if (border_width <= 0 || region_w <= 0 || region_h <= 0)
        return;
    if (!overlay)
        return;

    /* Clamp border_width to half the overlap width */
    int half_w = region_w / 2;
    if (border_width > half_w)
        border_width = half_w;

    float blend_range = (float)border_width * 2.0f / (float)region_w;

    for (int y = 0; y < region_h; y++) {
        uint8_t *dst = canvas + (region_y + y) * canvas_stride + region_x * 4;
        const uint8_t *src = overlay + (overlay_y + y) * overlay_stride + overlay_x * 4;

        for (int x = 0; x < region_w; x++) {
            float t = (float)x / (float)(region_w - 1); /* t ∈ [0, 1] */

            /* alpha: 0 = full img0 (overlay), 1 = full img1 (canvas) */
            float center = 0.5f;
            float alpha = (t - (center - blend_range / 2.0f)) / blend_range;

            if (alpha < 0.0f) alpha = 0.0f;
            if (alpha > 1.0f) alpha = 1.0f;

            uint8_t *d = dst + x * 4;
            const uint8_t *s = src + x * 4;

            /* Blend RGB, set output alpha to 255 */
            d[0] = (uint8_t)((1.0f - alpha) * s[0] + alpha * d[0] + 0.5f);
            d[1] = (uint8_t)((1.0f - alpha) * s[1] + alpha * d[1] + 0.5f);
            d[2] = (uint8_t)((1.0f - alpha) * s[2] + alpha * d[2] + 0.5f);
            d[3] = 255;
        }
    }
}
