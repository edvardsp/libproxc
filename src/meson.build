
proxc_sources = [
    'proxc.cpp',
    'work_stealing_policy.cpp'
]

proxc = shared_library('proxc',
    proxc_sources,
    include_directories : inc,
    dependencies : [thread_dep, boost_dep],
    install : true)

