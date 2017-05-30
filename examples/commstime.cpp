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
using ChanT = Chan< ItemT >;

constexpr std::size_t REPEAT = 100;
constexpr std::size_t RUNS = 50;

void chainer( ChanT::Rx in, ChanT::Tx out )
{
    for ( auto i : in ) {
        out << i + 1;
    }
}

void prefix( ChanT::Rx in, ChanT::Tx out )
{
    out << std::size_t{ 0 };
    for ( auto i : in ) {
        out << i;
    }
}

void delta( ChanT::Rx in, ChanT::Tx out, ChanT::Tx out_consume )
{
    for ( auto i : in ) {
        if ( out_consume << i ) {
            out << i;
        } else {
            break;
        }
    }
}

void consumer( ChanT::Rx in )
{
    for ( std::size_t i = 0; i < REPEAT; ++i ) {
        in();
    }
}

void commstime( std::size_t chain )
{
    std::size_t sum = 0;
    for ( std::size_t run = 0; run < RUNS; ++run ) {
        ChanT pre2del_ch, consume_ch;
        ChanVec< ItemT > chain_chs{ chain + 1 };

        std::vector< Process > chains;
        chains.reserve( chain );
        for ( std::size_t i = 0; i < chain; ++i ) {
            chains.emplace_back( chainer,
                chain_chs[i+1].move_rx(),
                chain_chs[i].move_tx() );
        }

        auto start = std::chrono::system_clock::now();

        parallel(
            proc( prefix, chain_chs[0].move_rx(), pre2del_ch.move_tx() ),
            proc( delta, pre2del_ch.move_rx(), chain_chs[chain].move_tx(), consume_ch.move_tx() ),
            proc( consumer, consume_ch.move_rx() ),
            proc_for( chains.begin(), chains.end() )
        );

        auto end = std::chrono::system_clock::now();
        std::chrono::duration< std::size_t, std::nano > diff = end - start;
        sum += diff.count();
    }
    std::cout << chain << "," << sum / RUNS << std::endl;
}

int main()
{
    for ( std::size_t chain = 1; chain < 50; chain += 1 ) {
        commstime( chain );
    }
    for ( std::size_t chain = 50; chain < 500; chain += 5 ) {
        commstime( chain );
    }
    for ( std::size_t chain = 500; chain <= 1000; chain += 10 ) {
        commstime( chain );
    }

    return 0;
}

