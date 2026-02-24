project(
  'inja',
  'cpp',
  version: '3.5.0',
  default_options: ['cpp_std=c++17', 'warning_level=3'],
  meson_version: '>=0.56'
)

pkgc = import('pkgconfig')

inja_dep = declare_dependency(
  include_directories: include_directories('include', 'third_party/include')
)


install_headers(
  'include/inja/config.hpp',
  'include/inja/environment.hpp',
  'include/inja/exceptions.hpp',
  'include/inja/function_storage.hpp',
  'include/inja/inja.hpp',
  'include/inja/json.hpp',
  'include/inja/json_fwd.hpp',
  'include/inja/lexer.hpp',
  'include/inja/node.hpp',
  'include/inja/parser.hpp',
  'include/inja/renderer.hpp',
  'include/inja/statistics.hpp',
  'include/inja/template.hpp',
  'include/inja/throw.hpp',
  'include/inja/token.hpp',
  'include/inja/utils.hpp',
  subdir: 'inja'
)

pkgc.generate(
    version: meson.project_version(),
    name: 'inja',
    description: 'Template engine for modern C++',
    url: 'https://github.com/pantor/inja',
    requires: ['nlohmann_json'],
    dataonly: true,
)


run_command(
  find_program('scripts/update_single_include.sh'),
  check: true
)


if get_option('build_tests')
  test_flags = ['-D__TEST_DIR__=' + meson.project_source_root() / 'test']

  inja_test = executable(
    'inja_test',
    'test/test.cpp',
    dependencies: inja_dep,
    cpp_args: test_flags,
  )

  inja_single_test = executable(
    'inja_single_test',
    'test/test.cpp',
    'single_include/inja/inja.hpp',
    dependencies: [inja_dep],
    cpp_args: test_flags,
  )

  test('Inja unit test', inja_test)
  test('Inja single include test', inja_single_test)


  inja_benchmark = executable(
    'inja_benchmark',
    'test/benchmark.cpp',
    dependencies: inja_dep,
    cpp_args: test_flags,
  )
endif
