#include <glib.h>
#include <math.h>
#include "homography_config.h"

/* Test: parse a valid 2-image homography JSON */
static void test_parse_valid_2img(void)
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
    g_assert_cmpint(config->num_inputs, ==, 2);
    g_assert_cmpint(config->reference_index, ==, 0);
    g_assert_cmpint(config->pairs[0].target, ==, 1);
    g_assert_cmpint(config->pairs[0].reference, ==, 0);
    /* Translation matrix: h02 = 640 */
    g_assert_cmpfloat(config->pairs[0].homography.h[0][2], ==, 640.0f);

    stitcher_config_free(config);
}

/* Test: reject empty homographies array */
static void test_parse_empty_array(void)
{
    const char *json = "{ \"homographies\": [] }";

    GError *error = NULL;
    StitcherConfig *config = stitcher_config_parse_json(json, strlen(json), &error);

    g_assert_null(config);
    g_assert_nonnull(error);
    g_error_free(error);
}

/* Test: reject target == reference */
static void test_reject_same_target_ref(void)
{
    const char *json =
        "{"
        "  \"homographies\": ["
        "    {"
        "      \"images\": { \"target\": 0, \"reference\": 0 },"
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

    g_assert_null(config);
    g_assert_nonnull(error);
    g_error_free(error);
}

/* Test: homography inverse of identity is identity */
static void test_inverse_identity(void)
{
    float h[3][3] = {{1,0,0},{0,1,0},{0,0,1}};
    float h_inv[3][3];

    stitcher_homography_invert(h, h_inv);

    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            g_assert_cmpfloat(fabsf(h_inv[r][c] - h[r][c]), <=, 1e-5f);
}

/* Test: inverse of translation matrix */
static void test_inverse_translation(void)
{
    float h[3][3] = {{1,0,100},{0,1,50},{0,0,1}};
    float h_inv[3][3];

    stitcher_homography_invert(h, h_inv);

    g_assert_cmpfloat(fabsf(h_inv[0][2] - (-100.0f)), <=, 1e-4f);
    g_assert_cmpfloat(fabsf(h_inv[1][2] - (-50.0f)),  <=, 1e-4f);
    g_assert_cmpfloat(fabsf(h_inv[0][0] - 1.0f), <=, 1e-5f);
    g_assert_cmpfloat(fabsf(h_inv[1][1] - 1.0f), <=, 1e-5f);
}

/* Test: 3-image configuration */
static void test_parse_3img(void)
{
    const char *json =
        "{"
        "  \"homographies\": ["
        "    {"
        "      \"images\": { \"target\": 0, \"reference\": 1 },"
        "      \"matrix\": {"
        "        \"h00\": 1, \"h01\": 0, \"h02\": -640,"
        "        \"h10\": 0, \"h11\": 1, \"h12\": 0,"
        "        \"h20\": 0, \"h21\": 0, \"h22\": 1"
        "      }"
        "    },"
        "    {"
        "      \"images\": { \"target\": 2, \"reference\": 1 },"
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
    g_assert_cmpint(config->num_inputs, ==, 3);
    g_assert_cmpint(config->reference_index, ==, 1);

    stitcher_config_free(config);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/stitcher/homography/parse_valid_2img", test_parse_valid_2img);
    g_test_add_func("/stitcher/homography/parse_empty_array", test_parse_empty_array);
    g_test_add_func("/stitcher/homography/reject_same_target_ref", test_reject_same_target_ref);
    g_test_add_func("/stitcher/homography/inverse_identity", test_inverse_identity);
    g_test_add_func("/stitcher/homography/inverse_translation", test_inverse_translation);
    g_test_add_func("/stitcher/homography/parse_3img", test_parse_3img);

    return g_test_run();
}
