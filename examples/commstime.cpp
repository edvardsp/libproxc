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

#include <algorithm>
#include <chrono>
#include <iostream>

#include <proxc.hpp>

using namespace proxc;

using ItemT = std::size_t;
using Tx = channel::Tx< ItemT >;
using Rx = channel::Rx< ItemT >;

//  0   successor
//  |   | (++)  A
//  V   V       |
//  prefix -> delta -> consumer

void successor( Tx tx, Rx rx  )
{
    for ( auto i : rx ) {
        tx.send( i + 1 );
    }
}

void prefix( Tx tx, Rx rx )
{
    tx.send( 0 );
    for ( auto i : rx ) {
        tx.send( i );
    }
}

void delta( Tx tx, Rx rx, Tx consume )
{
    for ( auto i : rx ) {
        if ( consume.send( i ) != proxc::channel::OpResult::Ok ) {
            break;
        }
        tx.send( i );
    }
}

void consumer( Rx rx )
{
    const std::size_t num_iters = 10000;
    const std::size_t rep = 1000;
    ItemT item;
    std::size_t ns_per_chan_op = 0;
    for ( std::size_t r = 0; r < rep; ++r ) {
        auto start = std::chrono::steady_clock::now();
        for ( std::size_t i = 0; i < num_iters; ++i ) {
            rx.recv( item );
        }
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration< std::size_t, std::nano > diff = end - start;
        ns_per_chan_op += diff.count();
    }
    std::cout << ns_per_chan_op / ( num_iters * rep * 4 ) << "ns per chan op" << std::endl;
}

int main()
{
    auto ch = channel::create< int >();
    auto tx = channel::get_tx( ch );
    auto rx = channel::get_rx( ch );
    parallel(
        proc( [&tx]{        tx << 42; } ),
        proc( [&rx]{ int i; rx >> i; } )
    );

    auto chs = channel::create_n< ItemT >( 4 );
    auto txs = channel::get_tx( chs );
    auto rxs = channel::get_rx( chs );

    parallel(
        proc( successor, std::move( txs[2] ), std::move( rxs[1] ) ),
        proc( prefix,    std::move( txs[0] ), std::move( rxs[2] ) ),
        proc( delta,     std::move( txs[1] ), std::move( rxs[0] ),
                         std::move( txs[3] ) ),
        proc( consumer,  std::move( rxs[3] ) )
    );

    return 0;
}

