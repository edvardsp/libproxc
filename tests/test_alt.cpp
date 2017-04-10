
#include <proxc/config.hpp>

#include <proxc/alt/alt.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/op.hpp>

#include "setup.hpp"

using namespace proxc;

void test_it_works()
{
    auto up    = channel::sync::create< int >();
    auto down  = channel::sync::create< int >();
    auto left  = channel::sync::create< int >();
    auto right = channel::sync::create< int >();

    auto up_tx    = channel::get_tx( up );
    auto up_rx    = channel::get_rx( up );
    auto down_tx  = channel::get_tx( down );
    auto down_rx  = channel::get_rx( down );
    auto left_tx  = channel::get_tx( left );
    auto left_rx  = channel::get_rx( left );
    auto right_tx = channel::get_tx( right );
    auto right_rx = channel::get_rx( right );

    Alt()
        .recv(
            up_rx )
        .recv(
            down_rx,
            []( auto ) {
                // some work
            } )
        .recv_if( true,
            left_rx )
        .recv_if( true,
            right_rx,
            []( auto ) {
                // some work
            } )
        .send(
            up_tx, 1 )
        .send(
            down_tx, 2,
            []() {
                // some work
            } )
        .send_if( true,
            left_tx, 3 )
        .send_if( true,
            right_tx, 4,
            []() {

            } )
        .timeout( std::chrono::steady_clock::now() )
        .timeout( std::chrono::steady_clock::now(),
            [](){
                 // some work
            } )
        .timeout( std::chrono::milliseconds(100) )
        .timeout( std::chrono::milliseconds(100),
            [](){
                // some work
            } )
        .timeout_if( true,
            std::chrono::steady_clock::now() )
        .timeout_if( true,
            std::chrono::steady_clock::now(),
            [](){
                 // some work
            } )
        .timeout_if( true,
            std::chrono::milliseconds(100) )
        .timeout_if( true,
            std::chrono::milliseconds(100),
            [](){
                // some work
            } )
        .select();
}

int main()
{
    test_it_works();

    return 0;
}

