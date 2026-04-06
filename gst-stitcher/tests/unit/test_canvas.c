#include <glib.h>
#include "canvas.h"
#include "homography_config.h"

/* Test: two 640x480 images, identity transform → canvas = 640x480 */
static void test_canvas_identity_2img(void)
{
    const char *json =
        "{"
        "  \"homographies\": ["
        "    {"
        "      \"images\": { \"target\": 1, \"reference\": 0 },"
        "      \"matrix\": {"
        "        \"h00\": 1, \"h01\": 0, \"h02\": 0,"
        "        \"h10\": 0, \"h11\": 1, \"h12\": 0,"
        "        \"h20\": 0, \"h21\": 0, \"h22\": 1"
        "      }"
        "    }"
        "  ]"
        "}";

    GError *error = NULL;
    StitcherConfig *config = stitcher_config_parse_json(json, strlen(json), &error);
    g_assert_no_error(error);
    g_assert_nonnull(config);

    ImageSize sizes[2] = {{640, 480}, {640, 480}};
    StitcherCrop crops[2] = {{0,0,0,0}, {0,0,0,0}};

    CanvasInfo *ci = canvas_compute(config, sizes, crops);
    g_assert_nonnull(ci);
    /* Identity: both images at same location, canvas = 640x480 */
    g_assert_cmpint(ci->canvas_w, ==, 640);
    g_assert_cmpint(ci->canvas_h, ==, 480);

    canvas_free(ci);
    stitcher_config_free(config);
}

/* Test: two 640x480 images, image1 shifted right by 640px → canvas = 1280x480 */
static void test_canvas_translation_2img(void)
{
    const char *json =
        "{"
        "  \"homographies\": ["
        "    {"
        "      \"images\": { \"target\": 1, \"reference\": 0 },"
        "      \"matrix\": {"
        "        \"h00\": 1, \"h01\": 0, \"h02\": 640,"
        "        \"h10\": 0, \"h11\": 1, \"h12\": 0,"
        "        \"h20\": 0, \"h21\": 0, \"h22\": 1"
        "      }"
        "    }"
        "  ]"
        "}";

    GError *error = NULL;
    StitcherConfig *config = stitcher_config_parse_json(json, strlen(json), &error);
    g_assert_no_error(error);
    g_assert_nonnull(config);

    ImageSize sizes[2] = {{640, 480}, {640, 480}};
    StitcherCrop crops[2] = {{0,0,0,0}, {0,0,0,0}};

    CanvasInfo *ci = canvas_compute(config, sizes, crops);
    g_assert_nonnull(ci);
    /* Image 0 at (0,0), Image 1 at (640,0) → canvas 1280x480 */
    g_assert_cmpint(ci->canvas_w, ==, 1280);
    g_assert_cmpint(ci->canvas_h, ==, 480);

    /* Check image offsets */
    g_assert_cmpint(ci->images[0].offset_x, ==, 0);
    g_assert_cmpint(ci->images[1].offset_x, ==, 0);
    g_assert_cmpint(ci->images[0].warp_x, ==, 0);
    g_assert_cmpint(ci->images[1].warp_x, ==, 640);

    canvas_free(ci);
    stitcher_config_free(config);
}

/* Test: crop reduces effective image size */
static void test_canvas_with_crop(void)
{
    const char *json =
        "{"
        "  \"homographies\": ["
        "    {"
        "      \"images\": { \"target\": 1, \"reference\": 0 },"
        "      \"matrix\": {"
        "        \"h00\": 1, \"h01\": 0, \"h02\": 600,"
        "        \"h10\": 0, \"h11\": 1, \"h12\": 0,"
        "        \"h20\": 0, \"h21\": 0, \"h22\": 1"
        "      }"
        "    }"
        "  ]"
        "}";

    GError *error = NULL;
    StitcherConfig *config = stitcher_config_parse_json(json, strlen(json), &error);
    g_assert_no_error(error);
    g_assert_nonnull(config);

    ImageSize sizes[2] = {{640, 480}, {640, 480}};
    StitcherCrop crops[2] = {{0,0,0,0}, {10, 10, 0, 0}}; /* crop 10px from left+right on img1 */

    CanvasInfo *ci = canvas_compute(config, sizes, crops);
    g_assert_nonnull(ci);

    /* Image 1 effectively 620px wide (crop 10px left+right from 640px), starts at x=610 */
    /* Canvas = max(640, 610+620) = 1230 */
    g_assert_cmpint(ci->canvas_w, ==, 1230);

    canvas_free(ci);
    stitcher_config_free(config);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/stitcher/canvas/identity_2img", test_canvas_identity_2img);
    g_test_add_func("/stitcher/canvas/translation_2img", test_canvas_translation_2img);
    g_test_add_func("/stitcher/canvas/with_crop", test_canvas_with_crop);

    return g_test_run();
}
