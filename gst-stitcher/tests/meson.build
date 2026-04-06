test_dep = [glib_dep, json_glib_dep, libstitcher_dep]

# Unit tests
test_homography_config = executable('test_homography_config',
  'unit/test_homography_config.c',
  dependencies: test_dep,
)
test('homography_config', test_homography_config)

test_canvas = executable('test_canvas',
  'unit/test_canvas.c',
  dependencies: test_dep,
)
test('canvas', test_canvas)

test_warp_cpu = executable('test_warp_cpu',
  'unit/test_warp_cpu.c',
  dependencies: test_dep,
)
test('warp_cpu', test_warp_cpu)
