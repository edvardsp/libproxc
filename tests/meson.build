
tests = [
    ['simple',               ['simple.cpp']],
    ['work_steal_deque',     ['test_work_steal_deque.cpp']],
    ['thread_pool',          ['test_thread_pool.cpp']],
    ['work_stealing_policy', ['test_work_stealing_policy.cpp']],
    ['context',              ['test_context.cpp']],
    ['scheduler',            ['test_scheduler.cpp']],
    ['channel_sync',         ['test_channel_sync.cpp']],
    ['circular_array',       ['test_circular_array.cpp']],
    ['spsc_queue',           ['test_spsc_queue.cpp']],
    ['parallel',             ['test_parallel.cpp']],
    ['process',              ['test_process.cpp']],
    ['alt',                  ['test_alt.cpp']],
    ['timer',               ['test_timer.cpp']]
]

foreach t : tests
    exe = executable('test_' + t[0], t[1],
        include_directories : [inc, src, tst],
        dependencies : [boost_dep, thread_dep],
        link_with : proxc)
    test('test ' + t[0], exe)
endforeach

