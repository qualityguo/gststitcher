#include "gststitcherpad.h"

enum {
    PROP_PAD_0,
    PROP_PAD_CROP_LEFT,
    PROP_PAD_CROP_RIGHT,
    PROP_PAD_CROP_TOP,
    PROP_PAD_CROP_BOTTOM,
};

G_DEFINE_TYPE(GstStitcherPad, gst_stitcher_pad, GST_TYPE_AGGREGATOR_PAD)

static void gst_stitcher_pad_get_property(GObject *object, guint prop_id,
    GValue *value, GParamSpec *pspec)
{
    GstStitcherPad *pad = GST_STITCHER_PAD(object);
    switch (prop_id) {
    case PROP_PAD_CROP_LEFT:   g_value_set_int(value, pad->crop_left);   break;
    case PROP_PAD_CROP_RIGHT:  g_value_set_int(value, pad->crop_right);  break;
    case PROP_PAD_CROP_TOP:    g_value_set_int(value, pad->crop_top);    break;
    case PROP_PAD_CROP_BOTTOM: g_value_set_int(value, pad->crop_bottom); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);  break;
    }
}

static void gst_stitcher_pad_set_property(GObject *object, guint prop_id,
    const GValue *value, GParamSpec *pspec)
{
    GstStitcherPad *pad = GST_STITCHER_PAD(object);
    switch (prop_id) {
    case PROP_PAD_CROP_LEFT:   pad->crop_left   = g_value_get_int(value); break;
    case PROP_PAD_CROP_RIGHT:  pad->crop_right  = g_value_get_int(value); break;
    case PROP_PAD_CROP_TOP:    pad->crop_top    = g_value_get_int(value); break;
    case PROP_PAD_CROP_BOTTOM: pad->crop_bottom = g_value_get_int(value); break;
    default: G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);   break;
    }
}

static void gst_stitcher_pad_class_init(GstStitcherPadClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->get_property = gst_stitcher_pad_get_property;
    gobject_class->set_property = gst_stitcher_pad_set_property;

    g_object_class_install_property(gobject_class, PROP_PAD_CROP_LEFT,
        g_param_spec_int("left", "Left Crop", "Pixels to crop from left",
            0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(gobject_class, PROP_PAD_CROP_RIGHT,
        g_param_spec_int("right", "Right Crop", "Pixels to crop from right",
            0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(gobject_class, PROP_PAD_CROP_TOP,
        g_param_spec_int("top", "Top Crop", "Pixels to crop from top",
            0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property(gobject_class, PROP_PAD_CROP_BOTTOM,
        g_param_spec_int("bottom", "Bottom Crop", "Pixels to crop from bottom",
            0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void gst_stitcher_pad_init(GstStitcherPad *pad)
{
    pad->crop_left = 0;
    pad->crop_right = 0;
    pad->crop_top = 0;
    pad->crop_bottom = 0;
}
