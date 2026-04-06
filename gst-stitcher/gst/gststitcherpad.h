#ifndef GST_STITCHER_PAD_H
#define GST_STITCHER_PAD_H

#include <gst/gst.h>
#include <gst/base/gstaggregator.h>

G_BEGIN_DECLS

#define GST_TYPE_STITCHER_PAD (gst_stitcher_pad_get_type())
G_DECLARE_FINAL_TYPE(GstStitcherPad, gst_stitcher_pad, GST, STITCHER_PAD, GstAggregatorPad)

struct _GstStitcherPad {
    GstAggregatorPad parent;

    /* Crop properties */
    gint crop_left;
    gint crop_right;
    gint crop_top;
    gint crop_bottom;
};

G_END_DECLS

#endif /* GST_STITCHER_PAD_H */
