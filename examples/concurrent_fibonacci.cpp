
#include <iostream>
#include <sstream>

#include <proxc.hpp>

using namespace proxc;
template<typename T> using Tx = channel::Tx< T >;
template<typename T> using Rx = channel::Rx< T >;


void fib_0( Tx< std::size_t > out )
{
    std::stringstream ss1, ss2;
    ss1 << "start: 0" << std::endl;
    ss2 << "end: 0" << std::endl;
    std::cout << ss1.str();
    out.send( std::size_t{ 0 } );
    std::cout << ss2.str();
}

void fib_1( Tx< std::size_t > out1, Tx< std::size_t > out2 )
{
    std::stringstream ss1, ss2;
    ss1 << "start: 1" << std::endl;
    ss2 << "end: 1" << std::endl;
    std::cout << ss1.str();
    out1.send( std::size_t{ 1 } );
    if ( ! out2.is_closed() ) {
        out2.send( std::size_t{ 1 } );
    }
    std::cout << ss2.str();
}


void fib_n( std::size_t m, Tx< std::size_t > out1, Tx< std::size_t > out2,
            Rx< std::size_t > in1,  Rx< std::size_t > in2 )
{
    std::stringstream ss1, ss2;
    ss1 << "start: " << m << std::endl;
    ss2 << "end: " << m << std::endl;
    std::cout << ss1.str();
    std::size_t nm1{}, nm2{};
    in1.recv( nm1 );
    in2.recv( nm2 );
    std::size_t n = nm1 + nm2;
    out1.send( n );
    if ( ! out2.is_closed() ) {
        out2.send( n );
    }
    std::cout << ss2.str();
}

std::size_t fib( const std::size_t n )
{
    if      ( n == 0 ) { return 0; }
    else if ( n == 1 ) { return 1; }

    auto chs = channel::create_n< std::size_t >( 2 * n + 1 );
    auto txs = std::move( std::get<0>( chs ) );
    auto rxs = std::move( std::get<1>( chs ) );

    rxs[ 2 * n ].close();
    rxs[ 2 * n - 2 ].close();

    std::size_t result{};
    parallel(
        proc( fib_0, std::move( txs[ 0 ] ) ),
        proc( fib_1, std::move( txs[ 1 ] ), std::move( txs[ 2 ] ) ),
        proc_for( std::size_t{ 0 }, n - 1,
            [&txs,&rxs]( auto i ){
                fib_n( i+2, std::move( txs[ 2*i+3 ] ), std::move( txs[ 2*i+4 ] ),
                       std::move( rxs[ 2*i+0 ] ), std::move( rxs[ 2*i+1 ] ) );
            } ),
        proc(
            [&result]( Rx< std::size_t > rx ){
                std::stringstream ss1, ss2;
                ss1 << "start: last" << std::endl;
                ss2 << "end: last" << std::endl;
                std::cout << ss1.str();
                rx.recv( result );
                std::cout << ss2.str();
            }, std::move( rxs[ 2*n-1 ] )
        )
    );
    return result;
}

int main()
{
    const std::size_t n = 15;
    /* parallel( */
    /*     proc_for( std::size_t{ 0 }, n, */
    /*         []( auto i ) { */
                    auto result = fib( n );
                    std::cout << "Fib " << n << ": " << result << std::endl;
            /* } ) */
    /* ); */

    return 0;
}
