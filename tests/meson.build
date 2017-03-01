
testexe = executable('testexe', 'simple.cpp',
    include_directories : inc,
    link_with : proxc)
test('simple test', testexe)

test_aligned_alloc_exe = executable('test_aligned_alloc_exe', 'test_aligned_alloc.cpp',
    include_directories : [inc, src, tst],
    dependencies : thread_dep,
    link_with : proxc)
test('test aligned alloc', test_aligned_alloc_exe)

test_work_steal_deque_exe = executable('test_work_steal_deque', 'test_work_steal_deque.cpp',
    include_directories : [inc, src, tst],
    dependencies : thread_dep,
    link_with : proxc)
test('test work-steal deque', test_work_steal_deque_exe)

test_thread_pool_exe = executable('test_thread_pool', 'test_thread_pool.cpp',
    include_directories : [inc, src, tst],
    dependencies : thread_dep,
    link_with : proxc)
test('test thread-pool', test_thread_pool_exe)

