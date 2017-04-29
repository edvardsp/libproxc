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
#include <thread>

#include <proxc.hpp>

#include "setup.hpp"

using namespace proxc;

void test_egg_timer()
{
    auto dur = std::chrono::microseconds( 100 );
    timer::Egg egg{ dur };
    throw_assert( ! egg.expired(), "timer should not be expired" );

    for ( std::size_t i = 0; i < 100; ++i ) {
        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should be expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should keep being expired" );

        egg.reset();

        throw_assert( ! egg.expired(), "timer should not be expired after reset" );
        throw_assert( ! egg.expired(), "timer should still not be expired" );
    }
}

void test_repeat_timer()
{
    auto dur = std::chrono::microseconds( 1000 );
    timer::Repeat rep{ dur };

    for ( std::size_t i = 0; i < 100; ++i ) {
        throw_assert( ! rep.expired(), "timer should not be expired" );
        throw_assert( ! rep.expired(), "timer should not be expired" );
        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
    }
}

void test_xx_timer()
{
    auto tp = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
    timer::Xx xx{ tp };

    throw_assert( ! xx.expired(), "timer should not be expired" );
    std::this_thread::sleep_until( tp );
    throw_assert(   xx.expired(), "timer should be expired" );
    xx.reset();
    throw_assert(   xx.expired(), "reset should not do anything" );
    std::this_thread::sleep_until( tp );
    throw_assert(   xx.expired(), "timer should keep being expired" );
}

int main()
{
    test_egg_timer();
    /* test_repeat_timer(); */
    test_xx_timer();

    return 0;
}
