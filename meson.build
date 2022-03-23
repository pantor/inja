project(
  'inja',
  'cpp',
  version: '3.3.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=3'
  ],
  meson_version: '>=0.56'
)

run_command(
  find_program('scripts/update_single_include.sh'),
  check: true
)

if get_option('single-header')
  subdir('single_include')
  inja_dep = declare_dependency(
    include_directories: include_directories('single_include', 'third_party/include')
  )
else
  subdir('include')
  inja_dep = declare_dependency(
    include_directories: include_directories('include', 'third_party/include')
  )
endif

if get_option('tests')
  subdir('test')
endif
