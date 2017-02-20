
project('proxc', 'cpp',
    version : '2.0.0',
    license : 'MIT')
add_global_arguments('-std=c++1y', language: 'cpp')

thread_dep = dependency('threads')

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('tests')
subdir('examples')

pkg_mod = import('pkgconfig')
pkg_mod.generate(
    libraries : proxc,
    version : '2.0.0',
    name : 'libproxc',
    filebase : 'proxc',
    description : 'A concurrency library for C/C++.')

