
proxc_sources = ['proxc.cpp']

proxc = shared_library('proxc',
    proxc_sources,
    include_directories : inc,
    dependencies : thread_dep,
    install : true)

dev_exe = executable('dev_exe', 'main.cpp',
    include_directories : inc,
    link_with : proxc)
