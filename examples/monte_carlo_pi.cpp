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

#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t NUM_WORKERS = 8;
constexpr std::size_t NUM_ITERS = ~std::size_t{ 0 } - std::size_t{ 1 };

void monte_carlo_pi( Chan< double >::Tx out, std::size_t iters ) noexcept
{
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution< double > distr{ 0., 1. };

    std::size_t in_circle = 0;
    for ( std::size_t i = 0; i < iters; ++i ) {
        auto x = distr( rng );
        auto y = distr( rng );
        auto length = x * x + y * y;
        if ( length <= 1. ) {
            ++in_circle;
        }
        this_proc::yield();
    }
    out << ( 4. * in_circle ) / static_cast< double >( iters );
}

void calculate( std::array< Chan< double >::Rx, NUM_WORKERS > in, std::size_t workers ) noexcept
{
    double sum = 0.;
    std::for_each( in.begin(), in.end(),
        [&sum]( auto& rx ){ sum += rx(); });
    sum /= static_cast< double >( workers );
    std::cout << "pi: " << sum << std::endl;
}

int main()
{
    constexpr std::size_t iter_work = NUM_ITERS / NUM_WORKERS;
    ChanArr< double, NUM_WORKERS > chs;

    std::vector< Process > workers;
    workers.reserve( NUM_WORKERS );
    for ( std::size_t i = 0; i < NUM_WORKERS; ++i ) {
        workers.emplace_back( monte_carlo_pi, chs[i].move_tx(), iter_work );
    }

    parallel(
        proc_for( workers.begin(), workers.end() ),
        proc( calculate, chs.collect_rx(), NUM_WORKERS )
    );

    return 0;
}

