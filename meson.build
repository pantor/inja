project('inja', 'cpp', default_options : ['cpp_std=c++11'])

inja_inc = include_directories('src')

inja_dep = declare_dependency(
  include_directories : inja_inc
)

subdir('test')
