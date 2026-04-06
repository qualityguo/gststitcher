#include <gst/gst.h>
#include "gststitcher.h"

#define PACKAGE "gststitcher"

static gboolean plugin_init(GstPlugin *plugin)
{
    return gst_element_register(plugin, "gststitcher", GST_RANK_NONE,
        GST_TYPE_STITCHER);
}

/* Manually define plugin descriptor with proper visibility */
__attribute__((visibility("default")))
const GstPluginDesc gst_plugin_desc = {
    .major_version = GST_VERSION_MAJOR,
    .minor_version = GST_VERSION_MINOR,
    .name = "gststitcher",
    .description = "Video stitching plugin",
    .plugin_init = plugin_init,
    .version = "0.1.0",
    .license = "LGPL",
    .source = "gst-stitcher",
    .package = "gst-stitcher",
    .origin = "https://github.com/qualityguo/gststitcher",
    .release_datetime = NULL,
};

/* Export the register function */
__attribute__((visibility("default")))
gboolean gst_plugin_gststitcher_register(GstPlugin *plugin) {
    return plugin_init(plugin);
}

