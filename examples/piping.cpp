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

void elem( channel::Rx< int > in, channel::Tx< int > out )
{
    while ( in >> out );
}

int main()
{
    const std::size_t n = 100;
    auto chs = channel::create_n< int >( n + 1 );

    std::vector< Process > elems;
    for ( std::size_t i = 0; i < n; ++i ) {
        elems.emplace_back( elem,
            channel::get_rx_ind( chs, i ), channel::get_tx_ind( chs, i + 1 ) );
    }

    parallel(
        proc_for( elems.begin(), elems.end() ),
        proc( []( channel::Tx< int > start, channel::Rx< int > end ){
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
            channel::get_tx_ind( chs, 0 ), channel::get_rx_ind( chs, n ) )
    );

    return 0;
}

