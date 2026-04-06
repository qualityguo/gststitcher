#include "homography_config.h"
#include <json-glib/json-glib.h>
#include <string.h>
#include <math.h>

static gboolean parse_matrix(JsonObject *obj, float h[3][3], GError **error)
{
    const char *fields[3][3] = {
        {"h00", "h01", "h02"},
        {"h10", "h11", "h12"},
        {"h20", "h21", "h22"},
    };

    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) {
            if (!json_object_has_member(obj, fields[r][c])) {
                g_set_error(error, G_OPTION_ERROR, 0,
                    "Missing matrix field: %s", fields[r][c]);
                return FALSE;
            }
            h[r][c] = (float)json_object_get_double_member(obj, fields[r][c]);
        }
    }
    return TRUE;
}

static float mat3_det(const float h[3][3])
{
    return h[0][0] * (h[1][1] * h[2][2] - h[1][2] * h[2][1])
         - h[0][1] * (h[1][0] * h[2][2] - h[1][2] * h[2][0])
         + h[0][2] * (h[1][0] * h[2][1] - h[1][1] * h[2][0]);
}

void stitcher_homography_invert(const float h[3][3], float h_inv[3][3])
{
    float det = mat3_det(h);
    g_return_if_fail(fabsf(det) > 1e-6f);

    float inv_det = 1.0f / det;

    h_inv[0][0] =  (h[1][1] * h[2][2] - h[1][2] * h[2][1]) * inv_det;
    h_inv[0][1] =  (h[0][2] * h[2][1] - h[0][1] * h[2][2]) * inv_det;
    h_inv[0][2] =  (h[0][1] * h[1][2] - h[0][2] * h[1][1]) * inv_det;
    h_inv[1][0] =  (h[1][2] * h[2][0] - h[1][0] * h[2][2]) * inv_det;
    h_inv[1][1] =  (h[0][0] * h[2][2] - h[0][2] * h[2][0]) * inv_det;
    h_inv[1][2] =  (h[0][2] * h[1][0] - h[0][0] * h[1][2]) * inv_det;
    h_inv[2][0] =  (h[1][0] * h[2][1] - h[1][1] * h[2][0]) * inv_det;
    h_inv[2][1] =  (h[0][1] * h[2][0] - h[0][0] * h[2][1]) * inv_det;
    h_inv[2][2] =  (h[0][0] * h[1][1] - h[0][1] * h[1][0]) * inv_det;
}

static gboolean validate_config(StitcherConfig *config, GError **error)
{
    if (config->num_inputs < 2) {
        g_set_error(error, G_OPTION_ERROR, 0,
            "Need at least 2 inputs, got %d", config->num_inputs);
        return FALSE;
    }
    if (config->num_inputs > STITCHER_MAX_INPUTS) {
        g_set_error(error, G_OPTION_ERROR, 0,
            "Too many inputs: %d (max %d)", config->num_inputs, STITCHER_MAX_INPUTS);
        return FALSE;
    }

    int pair_count = config->num_inputs - 1;
    gboolean seen_target[STITCHER_MAX_INPUTS] = {FALSE};
    int ref = config->reference_index;
    gboolean ref_found = FALSE;

    for (int i = 0; i < pair_count; i++) {
        StitcherPairConfig *pair = &config->pairs[i];

        if (pair->target == pair->reference) {
            g_set_error(error, G_OPTION_ERROR, 0,
                "Pair %d: target == reference (%d)", i, pair->target);
            return FALSE;
        }
        if (pair->target < 0 || pair->target >= config->num_inputs) {
            g_set_error(error, G_OPTION_ERROR, 0,
                "Pair %d: target %d out of range [0, %d]",
                i, pair->target, config->num_inputs - 1);
            return FALSE;
        }
        if (pair->reference < 0 || pair->reference >= config->num_inputs) {
            g_set_error(error, G_OPTION_ERROR, 0,
                "Pair %d: reference %d out of range [0, %d]",
                i, pair->reference, config->num_inputs - 1);
            return FALSE;
        }
        if (seen_target[pair->target]) {
            g_set_error(error, G_OPTION_ERROR, 0,
                "Pair %d: duplicate target %d", i, pair->target);
            return FALSE;
        }
        seen_target[pair->target] = TRUE;

        if (pair->reference == ref)
            ref_found = TRUE;

        float det = mat3_det(pair->homography.h);
        if (fabsf(det) < 1e-6f) {
            g_set_error(error, G_OPTION_ERROR, 0,
                "Pair %d: degenerate homography matrix (det ≈ 0)", i);
            return FALSE;
        }

        stitcher_homography_invert(pair->homography.h, pair->homography_inv.h);
    }

    if (!ref_found) {
        g_set_error(error, G_OPTION_ERROR, 0,
            "No pair references the reference image (index %d)", ref);
        return FALSE;
    }

    return TRUE;
}

static StitcherConfig *parse_homographies(JsonObject *root, GError **error)
{
    if (!json_object_has_member(root, "homographies")) {
        g_set_error(error, G_OPTION_ERROR, 0, "Missing 'homographies' member");
        return NULL;
    }

    JsonArray *arr = json_object_get_array_member(root, "homographies");
    guint n_pairs = json_array_get_length(arr);

    if (n_pairs == 0) {
        g_set_error(error, G_OPTION_ERROR, 0, "'homographies' array is empty");
        return NULL;
    }

    StitcherConfig *config = g_new0(StitcherConfig, 1);
    config->num_inputs = (int)n_pairs + 1;

    /* Determine reference index: the one that appears as reference but never as target */
    gboolean is_target[STITCHER_MAX_INPUTS] = {FALSE};
    gboolean is_ref[STITCHER_MAX_INPUTS] = {FALSE};

    for (guint i = 0; i < n_pairs; i++) {
        JsonObject *pair_obj = json_array_get_object_element(arr, i);
        JsonObject *images = json_object_get_object_member(pair_obj, "images");

        config->pairs[i].target = (int)json_object_get_int_member(images, "target");
        config->pairs[i].reference = (int)json_object_get_int_member(images, "reference");

        if (config->pairs[i].target >= 0 && config->pairs[i].target < STITCHER_MAX_INPUTS)
            is_target[config->pairs[i].target] = TRUE;
        if (config->pairs[i].reference >= 0 && config->pairs[i].reference < STITCHER_MAX_INPUTS)
            is_ref[config->pairs[i].reference] = TRUE;

        JsonObject *matrix = json_object_get_object_member(pair_obj, "matrix");
        if (!parse_matrix(matrix, config->pairs[i].homography.h, error)) {
            stitcher_config_free(config);
            return NULL;
        }
    }

    /* Find reference: appears as reference but not as target */
    config->reference_index = -1;
    for (int i = 0; i < config->num_inputs; i++) {
        if (is_ref[i] && !is_target[i]) {
            config->reference_index = i;
            break;
        }
    }
    if (config->reference_index < 0) {
        g_set_error(error, G_OPTION_ERROR, 0,
            "Cannot determine reference image");
        stitcher_config_free(config);
        return NULL;
    }

    if (!validate_config(config, error)) {
        stitcher_config_free(config);
        return NULL;
    }

    return config;
}

StitcherConfig *stitcher_config_parse_json(const char *json_str, gsize length, GError **error)
{
    JsonParser *parser = json_parser_new();
    if (!json_parser_load_from_data(parser, json_str, length, error)) {
        g_object_unref(parser);
        return NULL;
    }

    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_set_error(error, G_OPTION_ERROR, 0, "JSON root is not an object");
        g_object_unref(parser);
        return NULL;
    }

    StitcherConfig *config = parse_homographies(json_node_get_object(root), error);
    g_object_unref(parser);
    return config;
}

StitcherConfig *stitcher_config_parse_file(const char *path, GError **error)
{
    gchar *contents = NULL;
    gsize length = 0;

    if (!g_file_get_contents(path, &contents, &length, error))
        return NULL;

    StitcherConfig *config = stitcher_config_parse_json(contents, length, error);
    g_free(contents);
    return config;
}

void stitcher_config_free(StitcherConfig *config)
{
    if (config)
        g_free(config);
}
