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
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t N = 100;

void elem( Chan< int >::Rx in, Chan< int >::Tx out )
{
    while ( in >> out );
}

int main()
{
    ChanArr< int, N > chs;

    std::vector< Process > elems;
    for ( std::size_t i = 0; i < N; ++i ) {
        elems.emplace_back( elem, chs[i].move_rx(), chs[i+1].move_tx() );
    }

    parallel(
        proc_for( elems.begin(), elems.end() ),
        proc( []( Chan< int >::Tx start, Chan< int >::Rx end ){
                int start_item = 1337;
                start << start_item;
                for ( std::size_t i = 0; i < 1000; ++i ) {
                    end >> start;
                }
                int end_item{};
                end >> end_item;

                std::cout << "Start item: " << start_item << std::endl;
                std::cout << "End item: " << end_item << std::endl;
            },
            chs[0].move_tx(), chs[N].move_rx() )
    );

    return 0;
}

