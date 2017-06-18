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
#include <array>
#include <chrono>
#include <iostream>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t ROUNDS = 100;

constexpr std::size_t MAX_ITER = 255;

constexpr double XMIN = -2.1;
constexpr double XMAX =  1.0;
constexpr double YMIN = -1.3;
constexpr double YMAX =  1.3;

struct MandelbrotData
{
    std::size_t line;
    std::vector< double > values;

    MandelbrotData() = default;
    // make non-copyable
    MandelbrotData( MandelbrotData const & ) = delete;
    MandelbrotData & operator = ( MandelbrotData const & ) = delete;
    // make moveable
    MandelbrotData( MandelbrotData && ) = default;
    MandelbrotData & operator = ( MandelbrotData && ) = default;
};

using DataChan = Chan< MandelbrotData >;

inline bool point_predicate( const double x, const double y ) noexcept
{
    return ( x * x + y * y ) < 4.;
}

void mandelbrot( const std::size_t line, const std::size_t dim, DataChan::Tx out )
{
    const double integral_x = (XMAX - XMIN) / static_cast< double >( dim );
    const double integral_y = (YMAX - YMIN) / static_cast< double >( dim );

    MandelbrotData data = { line, std::vector< double >( dim ) };

    double y = YMIN + line * integral_y;
    double x = XMIN;

    for ( std::size_t x_coord = 0; x_coord < dim; ++x_coord ) {
        double x1 = 0., y1 = 0.;
        std::size_t loop_count = 0;
        while ( loop_count < MAX_ITER && point_predicate( x1, y1 ) ) {
            ++loop_count;
            double x1_new = x1 * x1 - y1 * y1 + x;
            y1 = 2 * x1 * y1 + y;
            x1 = x1_new;
        }

        double value = static_cast< double >( loop_count ) / static_cast< double >( MAX_ITER );
        data.values[ x_coord ] = value;
        x += integral_x;
    }

    out << std::move( data );
}

void consumer( const std::size_t dim, std::vector< DataChan::Rx > ins )
{
    std::vector< std::vector< double > > results( dim );
    for ( auto& in : ins ) {
        auto data = in();
        results[ data.line ] = std::move( data.values );
    }
}

void mandelbrot_program( std::size_t dim )
{
    std::size_t sum = 0;

    for ( std::size_t round = 0; round < ROUNDS; ++round ) {
        ChanVec< MandelbrotData > data_chs{ dim };

        std::vector< Process > workers;
        workers.reserve( dim );
        for ( std::size_t i = 0; i < dim; ++i ) {
            workers.emplace_back( mandelbrot, i, dim, data_chs[i].move_tx() );
        }

        auto start = std::chrono::system_clock::now();
        parallel(
            proc_for( workers.begin(), workers.end() ),
            proc( consumer, dim, data_chs.collect_rx() )
        );
        auto stop = std::chrono::system_clock::now();

        std::chrono::duration< std::size_t, std::nano > diff = stop - start;
        sum += diff.count();
    }

    std::cout << dim << ", " << sum / ROUNDS << std::endl;
}

int main()
{
    constexpr std::size_t dim = 100;
    mandelbrot_program( dim );
    return 0;
}

