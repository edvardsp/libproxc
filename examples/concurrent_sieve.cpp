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

#include <proxc.hpp>

using namespace proxc;
template<typename T> using Tx = channel::Tx< T >;
template<typename T> using Rx = channel::Rx< T >;

void generate( Tx< long > out, Rx< long > ex )
{
    long i = 2;
    while ( ex ) {
        out << i++;
    }
}

void filter( Rx< long > in, Tx< long > out )
{
    long prime{};
    in >> prime;
    for ( auto i : in ) {
        if ( i % prime != 0 ) {
            out << i;
        }
    }
}

int main()
{
    const long n = 4'000;
    auto ex_ch = channel::create< long >();
    auto chans = channel::create_n< long >( n );

    std::vector< Process > filters;
    filters.reserve( n - 1 );
    for ( std::size_t i = 0; i < n - 1; ++i ) {
        filters.emplace_back( filter,
            channel::get_rx_ind( chans, i ), channel::get_tx_ind( chans, i + 1 )
        );
    }

    auto start = std::chrono::steady_clock::now();

    parallel(
        proc( generate, channel::get_tx_ind( chans, 0 ), channel::get_rx( ex_ch ) ),
        proc_for( filters.begin(), filters.end() ),
        proc( [start,n]( Rx< long > in, Tx< long > ){
                long prime{};
                in >> prime;
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration< double, std::micro > diff = end - start;
                std::cout << n << "th prime is " << prime << std::endl;
                std::cout << "each prime took approx " << diff.count() / n << "us" << std::endl;
            },
            channel::get_rx_ind( chans, n - 1 ), channel::get_tx( ex_ch ) )
    );

    return 0;
}
