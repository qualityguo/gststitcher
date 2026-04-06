#ifndef GST_STITCHER_H
#define GST_STITCHER_H

#include <gst/gst.h>
#include <gst/base/gstaggregator.h>
#include "gststitcherpad.h"
#include "stitcher_types.h"
#include "warp_backend.h"
#include "homography_config.h"
#include "canvas.h"
#include "blender.h"

G_BEGIN_DECLS

#define GST_TYPE_STITCHER (gst_stitcher_get_type())
G_DECLARE_FINAL_TYPE(GstStitcher, gst_stitcher, GST, STITCHER, GstAggregator)

struct _GstStitcher {
    GstAggregator parent;

    /* Properties */
    gchar *homography_list;  /* inline JSON */
    gchar *homography_file;  /* JSON file path */
    gchar *map_file;         /* Map file path */
    gint border_width;
    gint output_width;       /* -1 = auto */
    gint output_height;      /* -1 = auto */

    /* Runtime state */
    gboolean initialized;
    gboolean use_map_mode;   /* TRUE if using map file mode */
    StitcherConfig *config;
    CanvasInfo *canvas_info;
    OverlapInfo *overlaps;
    const StitcherBackend *backend;
    void *backend_ctx;
    MapFileDataV2 *map_data;   /* Map file v2 data (owned by element) */
};

G_END_DECLS

#endif /* GST_STITCHER_H */
