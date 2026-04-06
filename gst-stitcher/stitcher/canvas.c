#include "canvas.h"
#include <math.h>
#include <float.h>
#include <stdio.h>
#include <glib.h>

typedef struct {
    float x, y;
} PointF;

static void transform_point(const float h[3][3], float x, float y, PointF *out)
{
    float w = h[2][0] * x + h[2][1] * y + h[2][2];
    out->x = (h[0][0] * x + h[0][1] * y + h[0][2]) / w;
    out->y = (h[1][0] * x + h[1][1] * y + h[1][2]) / w;
}

static void transform_corners(const float h[3][3], int w, int h_img,
    int cl, int cr, int ct, int cb, PointF corners[4])
{
    int x0 = cl, y0 = ct;
    int x1 = w - cr, y1 = h_img - cb;

    float pts[4][2] = {
        {(float)x0, (float)y0},
        {(float)x1, (float)y0},
        {(float)x0, (float)y1},
        {(float)x1, (float)y1},
    };
    for (int i = 0; i < 4; i++)
        transform_point(h, pts[i][0], pts[i][1], &corners[i]);
}

static void expand_bbox(const PointF corners[4],
    float *min_x, float *min_y, float *max_x, float *max_y)
{
    for (int i = 0; i < 4; i++) {
        if (corners[i].x < *min_x) *min_x = corners[i].x;
        if (corners[i].y < *min_y) *min_y = corners[i].y;
        if (corners[i].x > *max_x) *max_x = corners[i].x;
        if (corners[i].y > *max_y) *max_y = corners[i].y;
    }
}

/* Identity matrix for reference image */
static const float identity[3][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
};

static const float *get_homography_for_image(const StitcherConfig *config, int img_index)
{
    if (img_index == config->reference_index)
        return (const float *)identity;

    for (int i = 0; i < config->num_inputs - 1; i++) {
        if (config->pairs[i].target == img_index)
            return (const float *)config->pairs[i].homography.h;
    }

    /* Image only appears as reference in pairs — use identity */
    return (const float *)identity;
}

CanvasInfo *canvas_compute(const StitcherConfig *config,
    const ImageSize *sizes,
    const StitcherCrop *crops)
{
    float min_x = FLT_MAX, min_y = FLT_MAX;
    float max_x = -FLT_MAX, max_y = -FLT_MAX;

    PointF all_corners[STITCHER_MAX_INPUTS][4];

    for (int i = 0; i < config->num_inputs; i++) {
        const float *h = get_homography_for_image(config, i);
        transform_corners((const float (*)[3])h,
            sizes[i].w, sizes[i].h,
            crops[i].left, crops[i].right, crops[i].top, crops[i].bottom,
            all_corners[i]);
        expand_bbox(all_corners[i], &min_x, &min_y, &max_x, &max_y);

        /* Debug: print corner coordinates */
        fprintf(stderr, "Image %d corners: (%.2f,%.2f) (%.2f,%.2f) (%.2f,%.2f) (%.2f,%.2f)\n",
                i, all_corners[i][0].x, all_corners[i][0].y,
                all_corners[i][1].x, all_corners[i][1].y,
                all_corners[i][2].x, all_corners[i][2].y,
                all_corners[i][3].x, all_corners[i][3].y);
    }

    float off_x = -min_x;
    float off_y = -min_y;
    int canvas_w = (int)ceilf(max_x - min_x);
    int canvas_h = (int)ceilf(max_y - min_y);

    CanvasInfo *info = g_new0(CanvasInfo, 1);
    info->canvas_w = canvas_w;
    info->canvas_h = canvas_h;
    info->num_images = config->num_inputs;

    for (int i = 0; i < config->num_inputs; i++) {
        PointF *c = all_corners[i];

        float img_min_x = c[0].x, img_max_x = c[0].x;
        float img_min_y = c[0].y, img_max_y = c[0].y;
        for (int j = 1; j < 4; j++) {
            if (c[j].x < img_min_x) img_min_x = c[j].x;
            if (c[j].x > img_max_x) img_max_x = c[j].x;
            if (c[j].y < img_min_y) img_min_y = c[j].y;
            if (c[j].y > img_max_y) img_max_y = c[j].y;
        }

        /* Calculate where source (0,0) maps to on the canvas */
        float src_origin_x, src_origin_y;
        if (i == config->reference_index) {
            /* Reference image: source (0,0) maps directly */
            src_origin_x = 0.0f;
            src_origin_y = 0.0f;
        } else {
            /* Find the forward homography for this image */
            const StitcherPairConfig *pair = NULL;
            for (int j = 0; j < config->num_inputs - 1; j++) {
                if (config->pairs[j].target == i) {
                    pair = &config->pairs[j];
                    break;
                }
            }

            if (pair) {
                /* Transform source (0,0) by forward homography to find where it maps in reference frame */
                PointF origin_ref;
                transform_point((const float (*)[3])pair->homography.h, 0.0f, 0.0f, &origin_ref);
                src_origin_x = origin_ref.x;
                src_origin_y = origin_ref.y;
            } else {
                /* Should not happen, but fallback to (0,0) */
                src_origin_x = 0.0f;
                src_origin_y = 0.0f;
            }
        }

        /* Warp region should start where source (0,0) maps to on the canvas */
        /* Use the integer offset (not floating-point) for consistency with map generation */
        /* Use ceil() to ensure warp starts at or after the valid position (not before) */
        int offset_x_int = (int)roundf(off_x);
        int offset_y_int = (int)roundf(off_y);
        int warp_x = (int)ceilf(src_origin_x + offset_x_int);
        int warp_y = (int)ceilf(src_origin_y + offset_y_int);

        /* Warp region extends to cover all transformed corners */
        int warp_r = (int)ceilf(img_max_x + off_x);
        int warp_b = (int)ceilf(img_max_y + off_y);

        fprintf(stderr, "Canvas compute img %d: src_origin=(%.2f,%.2f) off=(%.2f,%.2f) warp=(%d,%d,%d,%d)\n",
                i, src_origin_x, src_origin_y, off_x, off_y, warp_x, warp_y, warp_r - warp_x, warp_b - warp_y);

        info->images[i].offset_x = (int)roundf(off_x);
        info->images[i].offset_y = (int)roundf(off_y);
        info->images[i].warp_x = warp_x;
        info->images[i].warp_y = warp_y;
        info->images[i].warp_w = warp_r - warp_x;
        info->images[i].warp_h = warp_b - warp_y;
    }

    return info;
}

void canvas_free(CanvasInfo *info)
{
    if (info)
        g_free(info);
}
