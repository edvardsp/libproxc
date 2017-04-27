
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

