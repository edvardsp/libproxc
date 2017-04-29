# MIT License
# 
# Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

tests = [
    ['simple',               ['simple.cpp']],
    ['work_steal_deque',     ['test_work_steal_deque.cpp']],
    ['work_stealing_policy', ['test_work_stealing_policy.cpp']],
    ['context',              ['test_context.cpp']],
    ['scheduler',            ['test_scheduler.cpp']],
    ['channel_sync',         ['test_channel_sync.cpp']],
    ['circular_array',       ['test_circular_array.cpp']],
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

