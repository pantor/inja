project('inja', 'cpp', default_options: ['cpp_std=c++11'])


inja_dep = declare_dependency(
  include_directories: include_directories('include')
)

inja_single_dep = declare_dependency(
  include_directories: include_directories('single_include', 'include')
)


# Amalgamate inja header files
r = run_command('python3', 'amalgamate/amalgamate.py', '-c', 'amalgamate/config.json', '-s', 'include')
if r.returncode() != 0
  message(r.stdout().strip())
else
  message('Amalgamated inja header files.')
endif


subdir('test')
