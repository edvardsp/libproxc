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

#include <iostream>
#include <sstream>

#include <proxc.hpp>

using namespace proxc;

using ItemT = std::size_t;
using ChanT = Chan< ItemT >;

void fib_0( ChanT::Tx out )
{
    out << std::size_t{ 0 };
}

void fib_1( ChanT::Tx out1, ChanT::Tx out2 )
{
    parallel(
        proc( [&]{ out1 << std::size_t{ 1 }; } ),
        proc( [&]{ out2 << std::size_t{ 1 }; } )
    );
}


void fib_n( ChanT::Tx out1, ChanT::Tx out2,
            ChanT::Rx in1,  ChanT::Rx in2 )
{
    std::size_t n = in1() + in2();
    parallel(
        proc( [&]{ out1 << n; } ),
        proc( [&]{ out2 << n; } )
    );
}

std::size_t fib( const std::size_t n )
{
    if      ( n == 0 ) { return 0; }
    else if ( n == 1 ) { return 1; }

    ChanVec< ItemT > chs{ 2 * n + 1 };

    chs[2 * n - 0].ref_rx().close();
    chs[2 * n - 2].ref_rx().close();

    std::vector< Process > fibs;
    fibs.reserve( n - 1 );
    for ( std::size_t i = 0; i < n - 1; ++i ) {
        fibs.emplace_back( & fib_n,
            chs[2*i+3].move_tx(), chs[2*i+4].move_tx(),
            chs[2*i+0].move_rx(), chs[2*i+1].move_rx() );
    }

    std::size_t result{};
    parallel(
        proc( & fib_0, chs[0].move_tx() ),
        proc( & fib_1, chs[1].move_tx(), chs[2].move_tx() ),
        proc_for( fibs.begin(), fibs.end() ),
        proc( [&result]( ChanT::Rx rx ){ rx >> result; },
            chs[2*n-1].move_rx() )
    );

    return result;
}

void print_fib( std::size_t n )
{
    auto result = fib( n );
    std::stringstream stream;
    stream << "Fib " << n << ": " << result << std::endl;
    std::cout << stream.str();
}

int main()
{
    constexpr std::size_t n = 50;
    parallel(
        proc_for( std::size_t{ 0 }, n,
            & print_fib )
    );

    return 0;
}
