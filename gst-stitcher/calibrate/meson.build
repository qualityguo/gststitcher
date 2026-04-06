opencv_dep = dependency('opencv4', required: true,
  modules: ['core', 'imgcodecs', 'imgproc', 'features2d', 'calib3d'])

glib_dep = dependency('glib-2.0', required: true)

calibrate_sources = [
  'calibrate_main.cpp',
  'feature_matcher.cpp',
  'homography_solver.cpp',
  'json_writer.cpp',
  'map_generator.cpp',
]

executable('gst-stitcher-calibrate',
  calibrate_sources,
  dependencies: [opencv_dep, glib_dep],
  link_with: libstitcher,
  install: true,
)

executable('gst-stitcher-mapgen',
  'gst_stitcher_mapgen.cpp',
  'map_generator.cpp',
  dependencies: [opencv_dep, glib_dep],
  link_with: libstitcher,
  install: true,
)
