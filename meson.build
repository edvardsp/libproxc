
project('proxc', 'cpp',
    version : '2.0.0',
    license : 'MIT',
    default_options : ['cpp_std=c++14'])
add_global_arguments('-std=c++1y', '-fcolor-diagnostics', language: 'cpp')

thread_dep = dependency('threads')
boost_dep = dependency('boost',
    modules : ['system', 'coroutine'])

inc = include_directories('include')
src = include_directories('src')
tst = include_directories('tests')


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

#dev_exe = executable('dev_exe', 'src/main.cpp',
#    include_directories : inc,
#    dependencies : [boost_dep, thread_dep],
#    link_with : proxc,
#    build_by_default : true)

#run_target = custom_target('run_exe',
#    output : 'run',
#    input : [],
#    command : [dev_exe])

