#include <glib.h>
#include "warp_backend.h"
#include <string.h>

#define PIXEL_TOLERANCE 2

/* Test: identity warp should produce identical output */
static void test_warp_identity(void)
{
    const StitcherBackend *backend = stitcher_backend_cpu_get();
    void *ctx = backend->init(8, 8);

    /* 4x4 red image (RGBA) */
    uint8_t src[4 * 4 * 4];
    for (int i = 0; i < 4 * 4; i++) {
        src[i * 4 + 0] = 255; /* R */
        src[i * 4 + 1] = 0;   /* G */
        src[i * 4 + 2] = 0;   /* B */
        src[i * 4 + 3] = 255; /* A */
    }

    uint8_t dst[4 * 4 * 4];
    memset(dst, 0, sizeof(dst));

    float identity[3][3] = {{1,0,0},{0,1,0},{0,0,1}};

    backend->warp(ctx, src, 4, 4, 16, dst, 16, 0, 0, 4, 4, identity, 0, 0);

    for (int i = 0; i < 4 * 4; i++) {
        g_assert_cmpint(abs(dst[i * 4 + 0] - 255), <=, PIXEL_TOLERANCE);
        g_assert_cmpint(abs(dst[i * 4 + 1] - 0),   <=, PIXEL_TOLERANCE);
        g_assert_cmpint(abs(dst[i * 4 + 2] - 0),   <=, PIXEL_TOLERANCE);
        g_assert_cmpint(abs(dst[i * 4 + 3] - 255),  <=, PIXEL_TOLERANCE);
    }

    backend->cleanup(ctx);
}

/* Test: translate by (2, 1) pixels */
static void test_warp_translation(void)
{
    const StitcherBackend *backend = stitcher_backend_cpu_get();
    void *ctx = backend->init(8, 8);

    /* 4x4 blue image */
    uint8_t src[4 * 4 * 4];
    for (int i = 0; i < 4 * 4; i++) {
        src[i * 4 + 0] = 0;
        src[i * 4 + 1] = 0;
        src[i * 4 + 2] = 255;
        src[i * 4 + 3] = 255;
    }

    /* 8x8 canvas */
    uint8_t dst[8 * 8 * 4];
    memset(dst, 0, sizeof(dst));

    /* H_inv for translation (tx=2, ty=1) is translation (-2, -1) */
    float h_inv[3][3] = {{1,0,-2},{0,1,-1},{0,0,1}};

    /* Warp to position (2, 1) on the 8x8 canvas */
    backend->warp(ctx, src, 4, 4, 16, dst, 32, 2, 1, 4, 4, h_inv, 0, 0);

    /* Check: pixel (2, 1) should be blue */
    uint8_t *px = dst + 1 * 32 + 2 * 4;
    g_assert_cmpint(abs(px[2] - 255), <=, PIXEL_TOLERANCE);
    g_assert_cmpint(px[3], ==, 255);

    /* Check: pixel (0, 0) should be transparent (not written by this warp) */
    g_assert_cmpint(dst[3], ==, 0);

    backend->cleanup(ctx);
}

/* Test: warp with out-of-bounds source produces transparent pixels */
static void test_warp_out_of_bounds(void)
{
    const StitcherBackend *backend = stitcher_backend_cpu_get();
    void *ctx = backend->init(8, 8);

    /* 2x2 image */
    uint8_t src[2 * 2 * 4];
    memset(src, 0, sizeof(src));

    uint8_t dst[8 * 8 * 4];
    memset(dst, 0, sizeof(dst));

    /* Large translation: maps to source coordinates well outside 2x2 */
    float h_inv[3][3] = {{1,0,0},{0,1,0},{0,0,1}};

    /* Warp a 4x4 region using a 2x2 source — edges will be out of bounds */
    backend->warp(ctx, src, 2, 2, 8, dst, 32, 0, 0, 4, 4, h_inv, 0, 0);

    /* Pixels at (2,0), (0,2), (2,2) should be transparent */
    uint8_t *px20 = dst + 0 * 32 + 2 * 4;
    g_assert_cmpint(px20[3], ==, 0);

    uint8_t *px02 = dst + 2 * 32 + 0 * 4;
    g_assert_cmpint(px02[3], ==, 0);

    backend->cleanup(ctx);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/stitcher/warp_cpu/identity", test_warp_identity);
    g_test_add_func("/stitcher/warp_cpu/translation", test_warp_translation);
    g_test_add_func("/stitcher/warp_cpu/out_of_bounds", test_warp_out_of_bounds);

    return g_test_run();
}
