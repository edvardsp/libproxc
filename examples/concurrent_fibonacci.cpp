
#include <iostream>

#include <proxc.hpp>

using namespace proxc;
template<typename T> using Tx = channel::Tx< T >;
template<typename T> using Rx = channel::Rx< T >;

void fib_0( Tx< long > out2 )
{
    while( ! out2.is_closed() ) {
        out2.send( long{ 0 } );
    }
}

void fib_1( Tx< long > out1, Tx< long > out2 )
{
    while( ! out1.is_closed() || ! out2.is_closed() ) {
        Alt()
            .send( out1, long{ 1 } )
            .send( out2, long{ 1 } )
            .select();
    }
}

void fib_n( Tx< long > out1, Tx< long > out2,
            Rx< long > in1,  Rx< long > in2 )
{
    long nm1{}, nm2{};
    in1.recv( nm1 );
    in2.recv( nm2 );
    long n = nm1 + nm2;
    while( ! out1.is_closed() || ! out2.is_closed() ) {
        Alt()
            .send( out1, n )
            .send( out2, n )
            .select();
    }
}

int main()
{
    const long n = 80;

    auto chs = channel::create_n< long >( n * 2 + 1 );
    auto txs = std::move( std::get<0>( chs ) );
    auto rxs = std::move( std::get<1>( chs ) );

    parallel(
        proc( fib_0, std::move( txs[ 0 ] ) ),
        proc( fib_1, std::move( txs[ 1 ] ), std::move( txs[ 2 ] ) ),
        proc_for( long{ 0 }, n - 1,
            [&txs,&rxs]( auto i ){
                fib_n( std::move( txs[ 2*i+3 ] ), std::move( txs[ 2*i+4 ] ),
                       std::move( rxs[ 2*i+0 ] ), std::move( rxs[ 2*i+1 ] ) );
            } ),
        proc(
            [n]( Rx< long > rx ){
                long m{};
                rx.recv( m );
                std::cout << "Fib " << n << ": " << m << std::endl;
            }, std::move( rxs[ 2*n-1 ] )
        )
    );

    return 0;
}
