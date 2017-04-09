
#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/parallel.hpp>
#include <proxc/this_proc.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/op.hpp>

#include "setup.hpp"

using namespace proxc;
using OpResult = proxc::channel::OpResult;

void test_channel_sync_works()
{
    int item = 42;
    auto ch = channel::sync::create< decltype(item) >();
    parallel(
        proc( [ item ]( auto tx ) {
                auto res = tx.send( item );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch ) ),
        proc( [ item ]( auto rx ) {
                decltype(item) i;
                auto res = rx.recv( i );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( i, item, "items should match" );
            }, channel::get_rx( ch ) )
    );
}

void test_range_recv()
{
    std::vector< bool > items = { true, false, false, true };
    auto ch = channel::sync::create< bool >();
    parallel(
        proc( [ &items ]( auto tx ) {
                for ( auto i : items ) {
                    auto res = tx.send( i );
                    throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                }
            }, channel::get_tx( ch ) ),
        proc( [ &items ]( auto rx ) {
                std::size_t n = 0;
                for ( auto i : rx ) {
                    throw_assert_equ( i, items[n++], "items should match" );
                }
                throw_assert_equ( n, items.size(), "should have received all items" );
            }, channel::get_rx( ch ) )
    );
}

void test_timeout_one_side()
{
    auto tx_send_for   = channel::sync::create< std::string >();
    auto tx_send_until = channel::sync::create< std::string >();
    auto rx_recv_for   = channel::sync::create< std::string >();
    auto rx_recv_until = channel::sync::create< std::string >();
    auto duration = std::chrono::milliseconds( 10 );

    parallel(
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, channel::get_tx( tx_send_for ) ),
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, channel::get_tx( tx_send_until ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, channel::get_rx( rx_recv_for ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, channel::get_rx( rx_recv_until ) )
    );
}

void test_timeout_complete()
{
    auto ch_for   = channel::sync::create< std::string >();
    auto ch_until = channel::sync::create< std::string >();

    auto duration   = std::chrono::milliseconds( 10 );

    parallel(
        // channel duration timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_for ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_for ) ),

        // channel time_point timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_until ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_until ) )
    );
}

void test_timeout_tx_delayed()
{
    auto ch_for   = channel::sync::create< std::string >();
    auto ch_until = channel::sync::create< std::string >();

    auto delay      = std::chrono::milliseconds( 5 );
    auto duration   = std::chrono::milliseconds( 20 );

    parallel(
        // channel duration timeout
        proc( [duration,delay]( auto tx ) {
                std::string item = "const ref";
                this_proc::delay_for( delay );
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for(delay);
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_for ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_for ) ),

        // channel time_point timeout
        proc( [duration,delay]( auto tx ) {
                std::string item = "const ref";
                this_proc::delay_for( delay );
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for( delay );
                time_point = std::chrono::steady_clock::now() + duration;
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_until ) ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_until ) )
    );
}

void test_timeout_rx_delayed()
{
    auto ch_for   = channel::sync::create< std::string >();
    auto ch_until = channel::sync::create< std::string >();

    auto delay      = std::chrono::milliseconds( 5 );
    auto duration   = std::chrono::milliseconds( 20 );
    auto time_point = std::chrono::steady_clock::now() + duration;

    parallel(
        // channel duration timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_for ) ),
        proc( [duration,delay]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                this_proc::delay_for( delay );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for( delay );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_for ) ),

        // channel time_point timeout
        proc( [time_point]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_tx( ch_until ) ),
        proc( [time_point,delay]( auto rx ) {
                std::string item;
                this_proc::delay_for( delay );
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for( delay );
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, channel::get_rx( ch_until ) )
    );
}

int main()
{
    test_channel_sync_works();
    test_range_recv();
    test_timeout_one_side();
    test_timeout_complete();
    test_timeout_tx_delayed();
    test_timeout_rx_delayed();

    return 0;
}
