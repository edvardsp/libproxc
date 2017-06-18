/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <proxc/scheduling_policy/round_robin.hpp>

#include <proxc/detail/hook.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace scheduling_policy {
namespace detail {

namespace hook = proxc::detail::hook;

template<>
void RoundRobin::enqueue( runtime::Context * ctx ) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    ctx->link(ready_queue_);
}

template<>
runtime::Context * RoundRobin::pick_next() noexcept
{
    runtime::Context * next = nullptr;
    if ( ! ready_queue_.empty() ) {
        next = &ready_queue_.front();
        ready_queue_.pop_front();
        BOOST_ASSERT(next != nullptr);
        BOOST_ASSERT( ! next->is_linked< hook::Ready >());
    }
    return next;
}

template<>
bool RoundRobin::is_ready() const noexcept
{
    return ! ready_queue_.empty();
}

template<>
void RoundRobin::suspend_until(TimePointT const & time_point) noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    cnd_.wait_until(lk, time_point,
        [&](){ return flag_; });
    flag_ = false;
}

template<>
void RoundRobin::notify() noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    flag_ = true;
    lk.unlock();
    cnd_.notify_all();
}

} // namespace detail
} // namespace scheduling_policy
PROXC_NAMESPACE_END

