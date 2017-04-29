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

#include <chrono>
#include <iostream>
#include <initializer_list>
#include <string>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>

#include <boost/intrusive_ptr.hpp>

#include "setup.hpp"

using ms = std::chrono::milliseconds;
using Clock = proxc::ClockT;

using namespace proxc;

void test_scheduler_self()
{
    auto self = Scheduler::self();
    throw_assert( self != nullptr, "scheduler should not be nullptr" );
}

void test_scheduler_running()
{
    auto running = Scheduler::running();
    throw_assert( running != nullptr, "running ctx should not be nullptr" );
    throw_assert( running->is_type( proxc::Context::Type::Main ), "running ctx should be main" );

}

void test_sleep_and_join()
{
    std::vector< std::size_t > answer = { 0, 100, 200, 300, 400, 500 };
    std::vector< std::size_t > numbers;
    std::vector< boost::intrusive_ptr< Context > > procs;

    auto func = [&numbers](std::size_t milli) {
        Scheduler::self()->sleep_until( Clock::now() + ms(milli) );
        numbers.push_back(milli);
    };

    for (auto it = answer.rbegin(); it != answer.rend(); ++it) {
        auto p = Scheduler::make_work( func, *it );
        Scheduler::self()->commit( p.get() );
        procs.push_back( std::move( p ) );
    }

    for (auto& p : procs) {
        Scheduler::self()->join( p.get() );
    }

    throw_assert_equ( numbers.size(), answer.size(), "both vectors should be of equal size" );
    for(std::size_t i = 0; i < numbers.size(); ++i) {
        throw_assert_equ( numbers[i], answer[i], "numbers should be equal" );
    }
}

int main()
{
    test_scheduler_self();
    test_scheduler_running();
    test_sleep_and_join();

    return 0;
}

