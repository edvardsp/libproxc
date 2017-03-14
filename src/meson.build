
proxc_sources = [
    'proxc.cpp',
    'scheduler.cpp',
    'context.cpp',
    'scheduling_policy/round_robin.cpp',
    'scheduling_policy/work_stealing.cpp'
]

proxc = shared_library('proxc',
    proxc_sources,
    include_directories : inc,
    dependencies : [thread_dep, boost_dep],
    install : true)

