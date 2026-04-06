#ifndef STITCHER_TYPES_H
#define STITCHER_TYPES_H

#include <stdint.h>

#define STITCHER_MAX_INPUTS 16

typedef struct {
    float h[3][3];
} StitcherHomography;

typedef struct {
    int left;
    int right;
    int top;
    int bottom;
} StitcherCrop;

typedef struct {
    int target;
    int reference;
    StitcherHomography homography;
    StitcherHomography homography_inv;
} StitcherPairConfig;

typedef struct {
    int num_inputs;
    int reference_index;
    StitcherPairConfig pairs[STITCHER_MAX_INPUTS];
} StitcherConfig;

typedef struct {
    int canvas_w;
    int canvas_h;
    int num_images;
    struct {
        int offset_x;
        int offset_y;
        int warp_x;
        int warp_y;
        int warp_w;
        int warp_h;
    } images[STITCHER_MAX_INPUTS];
} CanvasInfo;

typedef struct {
    int x;
    int y;
    int w;
    int h;
} StitcherRect;

typedef struct {
    int num_regions;
    StitcherRect regions[STITCHER_MAX_INPUTS];
    int pair_indices[STITCHER_MAX_INPUTS]; /* which pair produces this overlap */
} OverlapInfo;

/* Map file v2 related types */

/*
 * ValidatedCoordinate - Quantized coordinate with explicit validation
 *
 * Uses 8 bytes per pixel (vs 16 bytes for float2)
 * Flags encode interpolation behavior to match CPU backend
 */
typedef struct {
    uint16_t x0;        /* Primary X coordinate (0 to src_w-1) */
    uint16_t y0;        /* Primary Y coordinate (0 to src_h-1) */
    uint8_t  fx;        /* Fractional X (0-255, represents 0.0-1.0) */
    uint8_t  fy;        /* Fractional Y (0-255, represents 0.0-1.0) */
    uint8_t  flags;     /* Validation flags (see below) */
    uint8_t  reserved;  /* Padding for alignment */
} ValidatedCoordinate;

/* ValidatedCoordinate flags */
#define COORD_FLAG_VALID_X1  (1 << 0)  /* If set: x1 = x0+1, else: x1 = x0 (edge) */
#define COORD_FLAG_VALID_Y1  (1 << 1)  /* If set: y1 = y0+1, else: y1 = y0 (edge) */
#define COORD_FLAG_IS_VALID  (1 << 2)  /* If set: coordinate is valid (in bounds) */

/*
 * MapFileDataV2 - Version 2 map file structure
 *
 * Uses JSON metadata + binary coordinate data format
 * No backward compatibility with v1
 */
typedef struct {
    /* Canvas dimensions */
    int32_t  canvas_w;
    int32_t  canvas_h;

    /* Number of input images */
    int32_t  num_inputs;

    /* Per-input warp regions */
    int32_t  *warp_x;      /* Warp region X offset for each input */
    int32_t  *warp_y;      /* Warp region Y offset for each input */
    int32_t  *warp_w;      /* Warp region width for each input */
    int32_t  *warp_h;      /* Warp region height for each input */

    /* Per-input source dimensions */
    int32_t  *input_w;     /* Source image width for each input */
    int32_t  *input_h;     /* Source image height for each input */

    /* Per-input crop parameters */
    int32_t  *crop_left;
    int32_t  *crop_right;
    int32_t  *crop_top;
    int32_t  *crop_bottom;

    /* Coordinate data - one ValidatedCoordinate array per input */
    ValidatedCoordinate **coords;  /* Coordinate mapping for each input */

    /* Metadata (from JSON) */
    char     *metadata_json;  /* Raw JSON metadata string */
} MapFileDataV2;

#endif /* STITCHER_TYPES_H */
