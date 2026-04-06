gst_plugin_sources = [
  'plugin.c',
  'gststitcher.c',
  'gststitcherpad.c',
]

gst_plugin_name = 'gststitcher'

gst_plugin = shared_library(gst_plugin_name,
  gst_plugin_sources,
  dependencies: [gst_dep, gst_base_dep, glib_dep, json_glib_dep, libstitcher_dep],
  install: true,
  install_dir: join_paths(get_option('libdir'), 'gstreamer-1.0'),
  # Force export of gst_plugin_desc symbol for GStreamer plugin loading
  link_args: [
    '-Wl,--export-dynamic-symbol=gst_plugin_desc',
    '-Wl,--export-dynamic-symbol=gst_plugin_gststitcher_register',
  ],
)
