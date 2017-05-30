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

#include <sstream>
#include <iostream>

#include <proxc.hpp>

using namespace proxc;

using ItemT = std::size_t;

void generate( Chan< ItemT >::Tx out, Chan< ItemT >::Rx ex )
{
    ItemT i{ 2 };
    while ( ex ) {
        out << i++;
    }
}

void filter( Chan< ItemT >::Rx in, Chan< ItemT >::Tx out )
{
    ItemT prime = in();
    for ( auto i : in ) {
        if ( i % prime != 0 ) {
            out << i;
        }
    }
}

int main(int argc, char *argv[])
{
    if ( argc != 2 ) { return -1; }
    std::size_t n;
    std::stringstream{ argv[1] } >> n;

    Chan< ItemT >    ex_ch;
    ChanVec< ItemT > chs{ n };

    std::vector< Process > filters;
    filters.reserve( n - 1 );
    for ( std::size_t i = 0; i < n - 1; ++i ) {
        filters.emplace_back( filter, chs[i].move_rx(), chs[i+1].move_tx() );
    }

    this_proc::yield();

    auto start = std::chrono::steady_clock::now();

    parallel(
        proc( generate, chs[0].move_tx(), ex_ch.move_rx() ),
        proc_for( filters.begin(), filters.end() ),
        proc( [start,n]( Chan< ItemT >::Rx in, Chan< ItemT >::Tx ){
                in();
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration< std::size_t, std::nano > diff = end - start;
                std::cout << n << "," << diff.count() / n << std::endl;
            },
            chs[n-1].move_rx(), ex_ch.move_tx() )
    );

    return 0;
}
