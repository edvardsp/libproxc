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
#include <vector>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/work_steal_deque.hpp>
#include <proxc/scheduling_policy/policy_base.hpp>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {
namespace detail {

struct Barrier
{
    using MutexT  = std::mutex;
    using CndVarT = std::condition_variable;

    MutexT     mtx_{};
    CndVarT    cv_{};
    bool       flag_{ false };

    void wait() noexcept
    {
        std::unique_lock< MutexT > lk{ mtx_ };
        cv_.wait( lk, [this]{ return flag_; } );
        flag_ = false;
    }

    void wait_until( std::chrono::steady_clock::time_point const & time_point ) noexcept
    {
        std::unique_lock< MutexT > lk{ mtx_ };
        cv_.wait_until( lk, time_point,
            [this]{ return flag_; } );
        flag_ = false;
    }

    void notify() noexcept
    {
        std::unique_lock< MutexT > lk{ mtx_ };
        flag_ = true;
        lk.unlock();
        cv_.notify_all();
    }
};

template<typename T>
class WorkStealingPolicy : public PolicyBase<T>
{
private:
    static std::size_t                            num_workers_;
    static std::vector< WorkStealingPolicy * >    work_stealers_;

    Barrier    barrier_{};

    std::size_t                   id_;
    proxc::WorkStealDeque< T >    deque_{};
    Scheduler::ReadyQueue         ready_queue_{};

    static void init_();

public:
    using TimePointT = typename PolicyBase<T>::TimePointT;

    WorkStealingPolicy();
    ~WorkStealingPolicy() {}

    // make non-copyable
    WorkStealingPolicy( WorkStealingPolicy const & )               = delete;
    WorkStealingPolicy & operator = ( WorkStealingPolicy const & ) = delete;

    // make non-movable
    WorkStealingPolicy( WorkStealingPolicy && )                    = delete;
    WorkStealingPolicy & operator = ( WorkStealingPolicy && )      = delete;

    // Added work stealing methods
    void reserve( std::size_t capacity ) noexcept;

    // Policy base interface methods
    void enqueue( T * ) noexcept;

    T * pick_next() noexcept;

    bool is_ready() const noexcept;

    void suspend_until( TimePointT const & ) noexcept;

    void notify() noexcept;

private:
    T * steal() noexcept;

    void signal_stealing() noexcept;

    std::size_t take_id() noexcept;
};

} // namespace detail

using WorkStealing = detail::WorkStealingPolicy<Context>;

} // namespace scheduling_policy

PROXC_NAMESPACE_END

