unit_test = executable(
  'inja-test',
  'src/unit.cpp',
  'src/unit-files.cpp',
  'src/unit-renderer.cpp',
  'src/unit-string-helper.cpp',
  dependencies: inja_dep,
  include_directories: include_directories('thirdparty')
)

test('Inja unit test', unit_test)
