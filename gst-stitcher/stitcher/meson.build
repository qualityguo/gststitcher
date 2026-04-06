libstitcher_sources = [
  'homography_config.c',
  'canvas.c',
  'blender.c',
  'warp_cpu.c',
  'blend_cpu.c',
  'map_loader.c',
  'warp_map.c',
]

libstitcher_headers = [
  'stitcher_types.h',
  'homography_config.h',
  'canvas.h',
  'blender.h',
  'warp_backend.h',
  'map_loader.h',
]

libstitcher = static_library('stitcher',
  libstitcher_sources,
  dependencies: [glib_dep, json_glib_dep, math_dep],
  include_directories: include_directories('.'),
)

libstitcher_dep = declare_dependency(
  link_with: libstitcher,
  include_directories: include_directories('.'),
)
