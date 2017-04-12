
#include <iostream>

#include <proxc.hpp>

using namespace proxc;
namespace chan = channel::sync;
template<typename T> using Tx = chan::Tx< T >;
template<typename T> using Rx = chan::Rx< T >;

void generate( Tx< long > out, Rx< long > ex )
{
    long i = 2;
    while ( ! ex.is_closed() ) {
        out.send( i++ );
    }
}

void filter( Rx< long > in, Tx< long > out )
{
    long prime;
    in.recv( prime );
    for ( auto i : in ) {
        if ( i % prime != 0 ) {
            out.send( i );
        }
    }
}

int main()
{
    const long n = 400;
    auto ex_ch = chan::create< long >();
    auto chans = chan::create_n< long >( n );
    auto txs = std::move( std::get<0>( chans ) );
    auto rxs = std::move( std::get<1>( chans ) );

    auto start = std::chrono::steady_clock::now();
    parallel(
        proc( generate, std::move( txs[0] ), channel::get_rx( ex_ch ) ),
        proc_for( static_cast< long >( 0 ), n-1,
            [&txs,&rxs]( auto i ) {
                filter( std::move( rxs[i] ), std::move( txs[i+1] ) );
            }
        ),
        proc(
            [start,n]( Rx< long > in, Tx< long > ex ){
                long prime;
                in.recv( prime );
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration< double, std::micro > diff = end - start;
                std::cout << n << "th prime is " << prime << std::endl;
                std::cout << "each prime took approx " << diff.count() / n << "us" << std::endl;
                ex.close();
            },
            std::move( rxs[n-1] ), channel::get_tx( ex_ch )
        )
    );

    return 0;
}
