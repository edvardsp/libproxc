
#include <chrono>
#include <iostream>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/parallel.hpp>
#include <proxc/process.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/async.hpp>
#include <proxc/channel/op.hpp>

#include "setup.hpp"

using namespace proxc;
namespace chan = channel::async;
template<typename T> using Tx = chan::Tx< T >;
template<typename T> using Rx = chan::Rx< T >;

void fn1()
{
    std::cout << "fn1" << std::endl;
}

void fn2()
{
    std::cout << "fn2" << std::endl;
}

void fn3()
{
    std::cout << "fn3" << std::endl;
}

void fn4( int i )
{
    std::cout << "fn4: " << i << std::endl;
}

void fn5( int x, int y )
{
    std::cout << "fn5: " << x << ", " << y << std::endl;
}

void generate( Tx< int > out, Rx< int > ex )
{
    int i = 2;
    while ( ! ex.is_closed() ) {
        out.send( i++ );
    }
}

void filter( Rx< int > in, Tx< int > out )
{
    int prime;
    in.recv( prime );
    for ( auto i : in ) {
        if ( i % prime != 0 ) {
            out.send( i );
        }
    }
}

void benchmark( Rx< int > in, Tx< int > ex )
{

}

int main()
{
    const int n = 400;
    auto chans = chan::create_n< int >( n );
    auto ex_ch = chan::create< int >();
    /* auto iota = iota_n( n-1 ); */

    auto start = std::chrono::steady_clock::now();
    parallel(
        proc( generate, channel::get_tx( chans[0] ), channel::get_rx( ex_ch ) ),
        proc_for( 0, n-1,
            [&chans]( auto i ) {
                filter( channel::get_rx( chans[i] ), channel::get_tx( chans[i+1] ) );
            }
        ),
        proc(
            [start,n]( Rx< int > in, Tx< int > ex ){
                int prime;
                in.recv( prime );
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration< double, std::micro > diff = end - start;
                std::cout << n << "th prime is " << prime << std::endl;
                std::cout << "each prime took approx " << diff.count() / n << "us" << std::endl;
                ex.close();
            },
            channel::get_rx( chans[n-1] ), channel::get_tx( ex_ch )
        )
    );

    /* Process p{ fn3 }; */
    /* parallel( */
    /*     Process{ fn1 } */
    /* ); */
    /* parallel( */
    /*     Process{ fn1 }, */
    /*     Process{ fn2 }, */
    /*     Process{ fn3 }, */
    /*     std::move( p ) */
    /* ); */
}
