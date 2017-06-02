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

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/scheduler.hpp>

PROXC_NAMESPACE_BEGIN
namespace this_proc {

Process::Id get_id() noexcept
{
    return runtime::Scheduler::running()->get_id();
}

void yield() noexcept
{
    runtime::Scheduler::self()->yield();
}

template<typename Rep, typename Period>
void delay_for( std::chrono::duration< Rep, Period > const & duration ) noexcept
{
    static_cast< void >(
        runtime::Scheduler::self()->sleep_until(
            std::chrono::steady_clock::now() + duration )
    );
}

template<typename Clock, typename Dur>
void delay_until( std::chrono::time_point< Clock, Dur > const & time_point ) noexcept
{
    static_cast< void >(
        runtime::Scheduler::self()->sleep_until( time_point )
    );
}

} // namespace this_proc
PROXC_NAMESPACE_END

