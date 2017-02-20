
proxc_sources = ['proxc.cpp']

proxc = shared_library('proxc',
    proxc_sources,
    include_directories : inc,
    dependencies : thread_dep,
    install : true)

boost_dep = dependency('boost',
    modules : ['system', 'coroutine'])

dev_exe = executable('dev_exe', 'main.cpp',
    include_directories : inc,
    dependencies : boost_dep,
    link_with : proxc)
