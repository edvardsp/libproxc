
project('proxc', 'cpp',
    version : '2.0.0',
    license : 'MIT',
    default_options : ['cpp_std=c++14'])
add_global_arguments(
    '-DBOOST_EXECUTION_CONTEXT=1',
    language: 'cpp')

cxx = meson.get_compiler('cpp')
if cxx.get_id() == 'clang' or cxx.get_id() == 'gcc'
    add_global_arguments('-fcolor-diagnostics', language: 'cpp')
endif

enable_segmented_stacks = get_option('enable-segmented-stacks')
if enable_segmented_stacks
    v = cxx.version().split('.')
    v2 = '.'.join([v[0], v[1]])
    clang_support = cxx.get_id() == 'clang' and v2.version_compare('>=3.4')
    gcc_support = cxx.get_id() == 'gcc' and v2.version_compare('>=4.7')
    if clang_support or gcc_support
        add_global_arguments(
            '-DBOOST_USE_SEGMENTED_STACKS',
            '-DPROXC_USE_SEGMENTED_STACKS',
            '-fsplit-stack',
            language: 'cpp')
    else
        error('Segmented stacks are only supported by Clang 3.4+ or GCC 4.7+.')
    endif
endif

enable_valgrind = get_option('enable-valgrind')
if enable_valgrind
    add_global_arguments(
        '-DBOOST_USE_VALGRIND',
        '-DPROXC_USE_VALGRIND',
        language: 'cpp')
endif

thread_dep = dependency('threads')
boost_dep = cxx.find_library('boost_context', required : true, dirs : '/usr/local/lib')
# boost_dep = dependency('boost',
#     modules : ['context'],
#     version : '>=1.61')

inc = include_directories('include')
src = include_directories('src')
tst = include_directories('tests')
exm = include_directories('examples')

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

