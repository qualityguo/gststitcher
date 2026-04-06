project('gst-stitcher', 'c', 'cpp',
  version: '0.1.0',
  meson_version: '>= 0.56',
  default_options: [
    'warning_level=2',
    'c_std=c11',
  ],
)

gst_version = '1.14'

cc = meson.get_compiler('c')

glib_dep = dependency('glib-2.0', version: '>= 2.48')
gst_dep = dependency('gstreamer-1.0', version: '>= ' + gst_version)
gst_base_dep = dependency('gstreamer-base-1.0', version: '>= ' + gst_version)
json_glib_dep = dependency('json-glib-1.0', version: '>= 1.2')
math_dep = cc.find_library('m', required: false)

cuda_enabled = get_option('cuda')
calibrate_enabled = get_option('calibrate')

subdir('stitcher')
subdir('gst')

if cuda_enabled
  subdir('cuda')
endif

if calibrate_enabled
  subdir('calibrate')
endif

subdir('tests')
