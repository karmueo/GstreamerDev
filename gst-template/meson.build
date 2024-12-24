project('gst-template', ['cpp', 'c'])

plugins_install_dir = join_paths(get_option('libdir'), 'gstreamer-1.0')

cc = meson.get_compiler('c')
cxx = meson.get_compiler('cpp')

gst_version = meson.project_version()

api_version = '1.0'

gst_dep = dependency(
  'gstreamer-1.0',
  required: true,
  fallback: ['gstreamer', 'gst_dep'],
)
gstbase_dep = dependency(
  'gstreamer-base-1.0',
  fallback: ['gstreamer', 'gst_base_dep'],
)

subdir('gst-plugin')
subdir('gst-app')

# 测试
subdir('tests')