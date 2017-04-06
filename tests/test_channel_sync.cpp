
#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/parallel.hpp>
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

void test_timeout_send()
{
    auto ch = channel::sync::create< std::string >();
    parallel(
        proc( []( auto tx ) {
                std::string item = "test";
                auto dur = std::chrono::milliseconds( 300 );
                auto res = tx.send_for( item, dur );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, channel::get_tx( ch ) )
    );
}

int main()
{
    test_channel_sync_works();
    test_range_recv();
    test_timeout_send();

    return 0;
}
