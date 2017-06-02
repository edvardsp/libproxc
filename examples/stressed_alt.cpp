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
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t NUM = 200;

void writer( Chan< int >::Tx tx )
{
    int n = 0;
    while ( ! tx.is_closed() ) {
        tx << n++;
    }
}

void reader( std::array< Chan< int >::Rx, NUM > rxs )
{
    constexpr std::size_t num_runs = 100;
    constexpr std::size_t num_iter = 1000;
    double total_us = 0.;

    timer::Egg egg{ std::chrono::milliseconds( 1 ) };
    for ( std::size_t run = 0; run < num_runs; ++run ) {
        auto start = std::chrono::steady_clock::now();
        for ( std::size_t iter = 0; iter < num_iter; ++iter ) {
            Alt()
                .recv_for( rxs.begin(), rxs.end(),
                    []( int ){} )
                .select();
        }
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration< double, std::micro > diff = end - start;
        total_us += diff.count();
    }
    std::cout << "Avg. us per alt: " << total_us / num_runs / num_iter << "us" << std::endl;
}

int main()
{
    ChanArr< int, NUM > chs;

    std::vector< Process > writers;
    writers.reserve( NUM );
    for ( std::size_t i = 0; i < NUM; ++i ) {
        writers.emplace_back( writer, chs[i].move_tx() );
    }

    parallel(
        proc_for( writers.begin(), writers.end() ),
        proc( reader, chs.collect_rx() )
    );

    return 0;
}
