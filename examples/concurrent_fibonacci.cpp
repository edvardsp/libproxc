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
template<typename T> using Tx = channel::Tx< T >;
template<typename T> using Rx = channel::Rx< T >;


void fib_0( Tx< std::size_t > out )
{
    out << std::size_t{ 0 };
}

void fib_1( Tx< std::size_t > out1, Tx< std::size_t > out2 )
{
    parallel(
        proc( [&out1]{ out1 << std::size_t{ 1 }; } ),
        proc( [&out2]{ out2 << std::size_t{ 1 }; } )
    );
}


void fib_n( Tx< std::size_t > out1, Tx< std::size_t > out2,
            Rx< std::size_t > in1,  Rx< std::size_t > in2 )
{
    std::size_t n = in1() + in2();
    parallel(
        proc( [&out1,n]{ out1 << n; } ),
        proc( [&out2,n]{ out2 << n; } )
    );
}

std::size_t fib( const std::size_t n )
{
    if      ( n == 0 ) { return 0; }
    else if ( n == 1 ) { return 1; }

    auto chs = channel::create_n< std::size_t >( 2 * n + 1 );

    channel::get_rx_ind( chs, 2 * n - 0 ).close();
    channel::get_rx_ind( chs, 2 * n - 2 ).close();

    std::vector< Process > fibs;
    fibs.reserve( n - 1 );
    for ( std::size_t i = 0; i < n - 1; ++i ) {
        fibs.emplace_back( & fib_n,
            channel::get_tx_ind( chs, 2 * i + 3 ), channel::get_tx_ind( chs, 2 * i + 4 ),
            channel::get_rx_ind( chs, 2 * i + 0 ), channel::get_rx_ind( chs, 2 * i + 1 )
        );
    }

    std::size_t result{};
    parallel(
        proc( & fib_0, channel::get_tx_ind( chs, 0 ) ),
        proc( & fib_1, channel::get_tx_ind( chs, 1 ), channel::get_tx_ind( chs, 2 ) ),
        proc_for( fibs.begin(), fibs.end() ),
        proc( [&result]( Rx< std::size_t > rx ){ rx >> result; }, channel::get_rx_ind( chs, 2 * n - 1 ) )
    );

    return result;
}

int main()
{
    const std::size_t n = 50;
    parallel(
        proc_for( std::size_t{ 0 }, n,
            []( auto i ) {
                auto result = fib( i );
                std::stringstream ss;
                ss << "Fib " << i << ": " << result << std::endl;
                std::cout << ss.str();
            } )
    );

    return 0;
}
