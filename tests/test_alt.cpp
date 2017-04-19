
#include <iostream>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/alt/alt.hpp>
#include <proxc/channel.hpp>
#include <proxc/parallel.hpp>

#include "setup.hpp"
#include "obj.hpp"

using namespace proxc;

void test_all_cases()
{
    auto tx_ch = channel::create< int >();
    auto rx_ch = channel::create< int >();

    auto tx = channel::get_tx( tx_ch );
    auto rx = channel::get_rx( rx_ch );

    int item = 42;

    Alt()
        .recv(
            rx )
        .recv(
            rx,
            []( auto ) {
                // some work
            } )
        .recv_if( true,
            rx )
        .recv_if( true,
            rx,
            []( auto ) {
                // some work
            } )
        .send(
            tx, 1 )
        .send(
            tx, item )
        .send(
            tx, 2,
            []() {
                // some work
            } )
        .send(
            tx, item,
            []() {
                // some work
            } )
        .send_if( true,
            tx, 3 )
        .send_if( true,
            tx, item)
        .send_if( true,
            tx, 4,
            []() {

            } )
        .send_if( true,
            tx, item,
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

void test_single_send_case()
{
    std::vector< int > ints = { 0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10 };
    auto ch = channel::create< int >();
    parallel(
        proc(
            [&ints]( channel::Tx< int > tx ) {
                for ( auto& i : ints ) {
                    Alt()
                        .send( tx, i )
                        .select();
                }
            }, channel::get_tx( ch ) ),
        proc(
            [&ints]( channel::Rx< int > rx ) {
                for ( auto& i : ints ) {
                    int item{};
                    auto res = rx.recv( item );
                    throw_assert( res == channel::OpResult::Ok, "OpResult should be Ok" );
                    throw_assert_equ( item, i, "items should match" );
                }
            }, channel::get_rx( ch ) )
    );
}

void test_single_recv_case()
{
    std::vector< int > ints = { 0, -1, 2, -3, 4, -5, 6, -7, 8, -9, 10 };
    auto ch = channel::create< int >();
    parallel(
        proc(
            [&ints]( channel::Tx< int > tx ) {
                for ( auto& i : ints ) {
                    auto res = tx.send( i );
                    throw_assert( res == channel::OpResult::Ok, "OpResult should be Ok" );
                }
            }, channel::get_tx( ch ) ),
        proc(
            [&ints]( channel::Rx< int > rx ) {
                for ( auto& i : ints ) {
                    Alt()
                        .recv( rx,
                            [&i]( int item ) {
                                throw_assert( item == i, "items should match" );
                            } )
                        .select();
                }
            }, channel::get_rx( ch ) )
    );
}

void test_two_alt_single_case()
{
    std::vector< int > ints = { 23, 1, -2, 0, 3232, 42 };
    auto ch = channel::create< int >();
    parallel(
        proc(
            [&ints]( channel::Tx< int > tx ) {
                for ( auto& i : ints ) {
                    Alt()
                        .send( tx, i )
                        .select();
                }
            }, channel::get_tx( ch ) ),
        proc(
            [&ints]( channel::Rx< int > rx ) {
                for ( auto& i : ints ) {
                    Alt()
                        .recv( rx,
                            [&i]( int item ) {
                                throw_assert_equ( item, i, "items should match" );
                            } )
                        .select();
                }
            }, channel::get_rx( ch ) )
    );
}

void test_alting_triangle()
{
    using T = Obj;
    auto ch1 = channel::create< T >();
    auto ch2 = channel::create< T >();
    auto ch3 = channel::create< T >();
    std::size_t num = 1000;

    parallel(
        proc( [num]( channel::Tx< T > tx, channel::Rx< T > rx )
        {
            std::size_t n_tx = 0, n_rx = 0;
            while ( n_tx < num || n_rx < num ) {
                Alt()
                    .recv_if( n_rx < num, rx,
                        [&n_rx]( Obj o ){ 
                            throw_assert_equ( o, n_rx++, "items should match" );
                        } )
                    .send_if( n_tx < num, tx, Obj{ n_tx },
                        [&n_tx]{ ++n_tx; } )
                    .select();
            }
        }, channel::get_tx( ch1 ), channel::get_rx( ch2 ) ),
        proc( [num]( channel::Tx< T > tx, channel::Rx< T > rx )
        {
            std::size_t n_tx = 0, n_rx = 0;
            while ( n_tx < num || n_rx < num ) {
                Alt()
                    .recv_if( n_rx < num, rx,
                        [&n_rx]( Obj o ){
                            throw_assert_equ( o, n_rx++, "items should match" );
                        } )
                    .send_if( n_tx < num, tx, Obj{ n_tx },
                        [&n_tx]{ ++n_tx; } )
                    .select();
            }
        }, channel::get_tx( ch2 ), channel::get_rx( ch3 ) ),
        proc( [num]( channel::Tx< T > tx, channel::Rx< T > rx )
        {
            std::size_t n_tx = 0, n_rx = 0;
            while ( n_tx < num || n_rx < num ) {
                Alt()
                    .recv_if( n_rx < num, rx,
                        [&n_rx]( Obj o ){
                            throw_assert_equ( o, n_rx++, "items should match" );
                        } )
                    .send_if( n_tx < num, tx, Obj{ n_tx },
                        [&n_tx]{ ++n_tx; } )
                    .select();
            }
        }, channel::get_tx( ch3 ), channel::get_rx( ch1 ) )
    );
}

void test_simple_ex()
{
    std::vector< int > ints1 = { 5, 12, -9, 45, 93, 1 };
    std::vector< int > ints2 = { -1, 2, -3, 4 };
    auto ch1 = channel::create< int >();
    auto ch2 = channel::create< int >();
    parallel(
        proc(
            [&ints1]( channel::Tx< int > tx ) {
                for ( auto& i : ints1 ) {
                    auto res = tx.send( i );
                    throw_assert( res == channel::OpResult::Ok, "OpResult should be Ok" );
                }
            }, channel::get_tx( ch1 ) ),
        proc(
            [&ints2]( channel::Tx< int > tx ) {
                for ( auto& i : ints2 ) {
                    auto res = tx.send( i );
                    throw_assert( res == channel::OpResult::Ok, "OpResult should be Ok" );
                }
            }, channel::get_tx( ch2 ) ),
        proc(
            [&ints1,&ints2]( channel::Rx< int > rx1, channel::Rx< int > rx2 ) {
                auto it1 = ints1.begin(), it2 = ints2.begin();
                while ( it1 != ints1.end() || it2 != ints2.end() ) {
                    Alt()
                        .recv(
                            rx1,
                            [&it1]( int item ) {
                                int i = *it1++;
                                throw_assert_equ( item, i, "items should match" );
                            } )
                        .recv(
                            rx2,
                            [&it2]( int item ) {
                                int i = *it2++;
                                throw_assert_equ( item, i, "items should match" );
                            } )
                        .select();
                }
            }, channel::get_rx( ch1 ), channel::get_rx( ch2 ) )
    );
}

int main()
{
    test_all_cases();
    test_single_send_case();
    test_single_recv_case();
    test_two_alt_single_case();
    test_alting_triangle();
    test_simple_ex();

    return 0;
}

