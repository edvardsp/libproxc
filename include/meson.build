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

install_headers('proxc.hpp')
install_headers(
    'proxc/alt.hpp',
    'proxc/channel.hpp',
    'proxc/config.hpp',
    'proxc/context.hpp',
    'proxc/parallel.hpp',
    'proxc/process.hpp',
    'proxc/scheduler.hpp',
    'proxc/spinlock.hpp',
    'proxc/timer.hpp',
    'proxc/this_proc.hpp',
    'proxc/traits.hpp',
    'proxc/version.hpp',
    'proxc/work_steal_deque.hpp',
    subdir : 'proxc')
install_headers(
    'proxc/alt/state.hpp',
    'proxc/alt/sync.hpp',
    'proxc/alt/choice_base.hpp',
    'proxc/alt/choice_send.hpp',
    'proxc/alt/choice_recv.hpp',
    subdir : 'proxc/alt')
install_headers(
    'proxc/channel/op_result.hpp',
    'proxc/channel/sync.hpp',
    'proxc/channel/tx.hpp',
    'proxc/channel/rx.hpp',
    'proxc/channel/op.hpp',
    subdir : 'proxc/channel')
install_headers(
    'proxc/scheduling_policy/policy_base.hpp',
    'proxc/scheduling_policy/round_robin.hpp',
    'proxc/scheduling_policy/work_stealing.hpp',
    subdir : 'proxc/scheduling_policy')
install_headers(
    'proxc/detail/circular_array.hpp',
    'proxc/detail/cpu_relax.hpp',
    'proxc/detail/delegate.hpp',
    'proxc/detail/hook.hpp',
    'proxc/detail/mpsc_queue.hpp',
    'proxc/detail/num_cpus.hpp',
    'proxc/detail/queue.hpp',
    subdir : 'proxc/detail')
