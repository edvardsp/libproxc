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

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {
namespace detail {

template<typename T>
class RoundRobinPolicy : public PolicyBase<T>
{
private:
    runtime::Scheduler::ReadyQueue      ready_queue_{};
    std::mutex                          mtx_{};
    std::condition_variable             cnd_{};
    bool                                flag_{ false };

public:
    using TimePointT = typename PolicyBase<T>::TimePointT;

    RoundRobinPolicy() = default;

    RoundRobinPolicy(RoundRobinPolicy const &)             = delete;
    RoundRobinPolicy & operator=(RoundRobinPolicy const &) = delete;

    void enqueue(T *) noexcept;

    T * pick_next() noexcept;

    bool is_ready() const noexcept;

    void suspend_until(TimePointT const &) noexcept;

    void notify() noexcept;
};

} // namespace detail

using RoundRobin = detail::RoundRobinPolicy< runtime::Context >;

} // namespace scheduling_policy

PROXC_NAMESPACE_END

