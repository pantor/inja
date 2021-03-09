project('inja', 'cpp', default_options: ['cpp_std=c++17'])


#option('build_tests', type: 'boolean', value: true)
#option('build_benchmark', type: 'boolean', value: true)


inja_dep = declare_dependency(
  include_directories: include_directories('include', 'third_party/include')
)



amalg_script = files('scripts/update_single_include.sh')

amalg_files = files(
  'include/inja/inja.hpp',
	'include/inja/renderer.hpp',
	'include/inja/environment.hpp',
)

amalg_tgt = run_target( 'amalg',
	command: amalg_script
)


inja_test = executable(
  'inja_test',
  'test/test.cpp',
  dependencies: inja_dep
)

inja_single_test = executable(
  'inja_single_test',
  'test/test.cpp',
  'single_include/inja/inja.hpp',
  dependencies: [inja_dep]
)


inja_benchmark = executable(
  'inja_benchmark',
  'test/benchmark.cpp',
  dependencies: inja_dep
)


test('Inja unit test', inja_test)
test('Inja single include test', inja_single_test)
