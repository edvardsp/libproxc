
#include <chrono>
#include <iostream>

#include <proxc/scheduler.hpp>
#include <proxc/channel/async.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/op.hpp>

using namespace proxc;
namespace chan = channel::async;

using ItemType = std::size_t;
using Tx = chan::Tx< ItemType >;
using Rx = chan::Rx< ItemType >;

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
    ItemType item;
    std::size_t ns_per_chan_op = 0;
    const std::size_t num_iters = 1000;
    for ( std::size_t i = 0; i < num_iters; ++i ) {
        auto start = std::chrono::steady_clock::now();
        rx.recv( item );
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration< std::size_t, std::nano > diff = end - start;
        ns_per_chan_op += diff.count() / 4;
    }
    std::cout << ns_per_chan_op / num_iters << "ns per chan op" << std::endl;

}

int main()
{
    auto a_ch = chan::create< ItemType >();
    auto b_ch = chan::create< ItemType >();
    auto c_ch = chan::create< ItemType >();
    auto d_ch = chan::create< ItemType >();

    auto self = proxc::Scheduler::self();
    auto suc = self->make_work( successor,
        channel::get_tx( b_ch ),
        channel::get_rx( a_ch ) );
    auto pre = self->make_work( prefix,
        channel::get_tx( c_ch ),
        channel::get_rx( b_ch ) );
    auto del = self->make_work( delta,
        channel::get_tx( a_ch ),
        channel::get_rx( c_ch ),
        channel::get_tx( d_ch ) );
    auto con = self->make_work( consumer,
        channel::get_rx( d_ch ) );

    self->commit( suc.get() );
    self->commit( pre.get() );
    self->commit( del.get() );
    self->commit( con.get() );

    self->join( con.get() );

    return 0;
}

