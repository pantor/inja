unit_test = executable(
  'inja-test',
  'src/unit.cpp',
  'src/unit-files.cpp',
  'src/unit-renderer.cpp',
  'src/unit-string-helper.cpp',
  dependencies: inja_dep
)

unit_single_test = executable(
  'inja-single-test',
  'src/unit.cpp',
  'src/unit-files.cpp',
  'src/unit-renderer.cpp',
  'src/unit-string-helper.cpp',
  dependencies: inja_single_dep
)

inja_benchmark = executable(
  'inja_benchmark',
  'src/benchmark.cpp',
  dependencies: inja_dep
)

test('Inja unit test', unit_test)
test('Inja single unit test', unit_single_test)
