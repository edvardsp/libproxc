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

#include <proxc/context.hpp>
#include <proxc/scheduling_policy/work_stealing.hpp>

#include <chrono>

#include "setup.hpp"

using PolicyType = proxc::scheduling_policy::WorkStealing;
using WorkType = proxc::Context;
using ClockType = std::chrono::steady_clock;

void test_enqueuing_and_pick_next(PolicyType & policy)
{
    const std::size_t num_work = 1000;

    {
        auto work = policy.pick_next();
        throw_assert(work == nullptr, "work should be nullptr");
    }

    throw_assert( ! policy.is_ready(), "policy should not be ready" );

    policy.reserve(num_work);
    for (std::size_t i = 0; i < num_work; ++i) {
        policy.enqueue(new WorkType(proxc::context::work_type, [](void *){  }));
    }

    throw_assert(policy.is_ready(), "policy should be ready");

    for (std::size_t i = 0; i < num_work; ++i) {
        auto work = policy.pick_next();
        throw_assert(work != nullptr, "work should not be nullptr");
        delete work;
    }

    throw_assert( ! policy.is_ready(), "policy should not be ready");
}

void test_suspend_until(PolicyType & policy)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto check_duration = [&policy](auto duration){
        auto start = ClockType::now();
        policy.suspend_until(ClockType::now() + duration);
        auto diff = ClockType::now() - start;
        throw_assert(diff > duration, "time suspended is not greater than duration");

        auto time_over = diff - duration;
        throw_assert(time_over < 300us, "time over suspension is greater than 200us => " << duration_cast<microseconds>(time_over).count() << "us");
    };

    check_duration(750us);
    check_duration(1000us);
    check_duration(1250us);
    check_duration(1500us);
    check_duration(1750us);
    check_duration(2000us);
}

int main()
{
    /* proxc::scheduling_policy::WorkStealing policy{ 1 }; */

    /* test_enqueuing_and_pick_next(policy); */
    /* test_suspend_until(policy); */

    return 0;
}
