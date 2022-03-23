inja_test = executable(
  'inja_test',
  'test.cpp',
  dependencies: inja_dep
)

inja_benchmark = executable(
  'inja_benchmark',
  'benchmark.cpp',
  dependencies: inja_dep
)

test('inja_test', inja_test, workdir: meson.project_build_root())
benchmark('inja_benchmark', inja_benchmark, workdir: meson.project_build_root())
