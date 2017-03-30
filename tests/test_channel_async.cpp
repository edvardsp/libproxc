
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/channel/async.hpp>
#include <proxc/channel/op.hpp>

#include "setup.hpp"

using namespace proxc;
using ms = std::chrono::milliseconds;

using ItemType = std::string;
template<typename T>
using ChanType = channel::async::detail::AsyncChannel< T >;

void test_raw_channel()
{
    ChanType< ItemType > ch;



}

void test_it_works()
{
    std::vector< int > answer = { 45, 3212, 32, -3, 0, -1221 };
    std::vector< int > ints;

    auto ch = channel::async::create< int >();
    auto p1 = Scheduler::make_work(
        [&ch,&answer]( auto tx ) {
            for ( auto& i : answer ) {
                tx.send( i );
            }
        },
        channel::get_tx( ch )
        /* std::move( std::get<0>( ch ) ) */
    );
    auto p2 = Scheduler::make_work(
        [&ch,&ints,&p1]( auto rx ) {
            Scheduler::self()->join( p1.get() );
            for ( auto i : rx ) {
                ints.push_back( std::move( i ) );
            }
        },
        channel::get_rx( ch )
        /* std::move( std::get<1>( ch ) ) */
    );

    Scheduler::self()->commit( p2.get() );
    Scheduler::self()->commit( p1.get() );

    Scheduler::self()->join( p2.get() );

    throw_assert_equ( ints.size(), answer.size(), "both vectors should have equal size" );
    for ( std::size_t i = 0; i < answer.size(); ++i ) {
        throw_assert_equ( ints[i], answer[i], "items should match" );
    }
}

int main()
{
    test_raw_channel();
    test_it_works();

    return 0;
}
