#ifndef HOMOGRAPHY_CONFIG_H
#define HOMOGRAPHY_CONFIG_H

#include "stitcher_types.h"
#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

StitcherConfig *stitcher_config_parse_json(const char *json_str, gsize length, GError **error);
StitcherConfig *stitcher_config_parse_file(const char *path, GError **error);
void            stitcher_config_free(StitcherConfig *config);

void stitcher_homography_invert(const float h[3][3], float h_inv[3][3]);

#ifdef __cplusplus
}
#endif

#endif /* HOMOGRAPHY_CONFIG_H */
