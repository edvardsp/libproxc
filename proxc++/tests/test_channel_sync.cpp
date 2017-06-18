/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <chrono>
#include <string>
#include <tuple>
#include <vector>

#include <proxc/config.hpp>
#include <proxc.hpp>

#include "setup.hpp"

using namespace proxc;
using OpResult = proxc::channel::OpResult;

class Obj
{
private:
    std::size_t value_;

public:
    Obj()
        : value_{}
    {}

    Obj( std::size_t value )
        : value_{ value }
    {}

    Obj( Obj const & ) = default;
    Obj & operator = ( Obj const & ) = default;

    Obj( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
    }
    Obj & operator = ( Obj && other )
    {
        std::swap( value_, other.value_ );
        other.value_ = std::size_t{};
        return *this;
    }

    bool operator == ( Obj const & other )
    { return value_ == other.value_; }

    template<typename CharT, typename TraitsT>
    friend std::basic_ostream< CharT, TraitsT > &
    operator << ( std::basic_ostream< CharT, TraitsT > & out, Obj const & obj )
    { return out << "Obj{" << obj.value_ << "}"; }
};

void test_item_copy_move()
{
    using ObjType = Obj;
    Chan< ObjType > ch;

    const ObjType msg{ 1337 };
    parallel(
        proc(
            [&msg]( channel::Tx< ObjType > tx ) {
                OpResult res;
                auto duration = std::chrono::milliseconds( 10 );
                ObjType s{ msg };

                res = tx.send( s );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, msg, "item should have been copied" );

                res = tx.send( std::move( s ) );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, ObjType{}, "item should have been moved" );

                s = msg;

                res = tx.send_for( s, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, msg, "item should have been copied" );

                res = tx.send_for( std::move( s ), duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, ObjType{}, "item should have been moved" );

                s = msg;

                res = tx.send_until( s, std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, msg, "item should have been copied" );

                res = tx.send_until( std::move( s ), std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( s, ObjType{}, "item should have been moved" );
            },
            ch.move_tx() ),
        proc(
            [&msg]( channel::Rx< ObjType > rx ) {
                for ( auto i : rx ) {
                    throw_assert_equ( i, msg, "strings should match" );
                }
            },
            ch.move_rx() )
    );
}

void test_item_copy_move_timeout()
{
    Chan< std::string > ch;
    auto tx = ch.move_tx();
    auto rx = ch.move_rx();

    auto duration = std::chrono::milliseconds( 1 );
    OpResult res;
    const std::string msg = "unique message";
    std::string s{ msg };
    std::string m{};

    res = tx.send_for( s, duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
    throw_assert_equ( s, msg, "item should have been copied" );

    res = tx.send_for( std::move( s ), duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
    throw_assert_equ( s, msg, "item should have not been moved" );

    res = tx.send_until( s, std::chrono::steady_clock::now() + duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
    throw_assert_equ( s, msg, "item should have been copied" );

    res = tx.send_until( std::move( s ), std::chrono::steady_clock::now() + duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
    throw_assert_equ( s, msg, "item should have not been moved" );

    res = rx.recv_for( m, duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );

    res = rx.recv_until( m, std::chrono::steady_clock::now() + duration );
    throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
}

void test_channel_sync_works()
{
    int item = 42;
    Chan< decltype(item) > ch;
    parallel(
        proc( [ &item ]( auto tx ) {
                auto res = tx.send( item );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            },
            ch.move_tx() ),
        proc( [ &item ]( auto rx ) {
                decltype(item) i{};
                auto res = rx.recv( i );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                throw_assert_equ( i, item, "items should match" );
            },
            ch.move_rx() )
    );
}

void test_range_recv()
{
    std::vector< bool > items = { true, false, false, true };
    Chan< bool > ch;
    parallel(
        proc( [ &items ]( auto tx ) {
                for ( auto i : items ) {
                    auto res = tx.send( i );
                    throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                }
            }, ch.move_tx() ),
        proc( [ &items ]( auto rx ) {
                std::size_t n = 0;
                for ( auto i : rx ) {
                    throw_assert_equ( i, items[n++], "items should match" );
                }
                throw_assert_equ( n, items.size(), "should have received all items" );
            }, ch.move_rx() )
    );
}

void test_timeout_one_side()
{
    Chan< std::string > tx_send_for,
                        tx_send_until,
                        rx_recv_for,
                        rx_recv_until;
    auto duration = std::chrono::milliseconds( 10 );

    parallel(
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, tx_send_for.move_tx() ),
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, tx_send_until.move_tx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, rx_recv_for.move_rx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Timeout, "OpResult should be Timeout" );
            }, rx_recv_until.move_rx() )
    );
}

void test_timeout_complete()
{
    Chan< std::string > ch_for, ch_until;

    auto duration   = std::chrono::milliseconds( 10 );

    parallel(
        // channel duration timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_for.move_tx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_for.move_rx() ),

        // channel time_point timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = tx.send_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = tx.send_until( "movable", time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_until.move_tx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_until.move_rx() )
    );
}

void test_timeout_tx_delayed()
{
    Chan< std::string > ch_for, ch_until;

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
            }, ch_for.move_tx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_for.move_rx() ),

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
            }, ch_until.move_tx() ),
        proc( [duration]( auto rx ) {
                std::string item;
                auto time_point = std::chrono::steady_clock::now() + duration;
                auto res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                time_point = std::chrono::steady_clock::now() + duration;
                res = rx.recv_until( item, time_point );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_until.move_rx() )
    );
}

void test_timeout_rx_delayed()
{
    Chan< std::string > ch_for, ch_until;

    auto delay      = std::chrono::milliseconds( 1 );
    auto duration   = std::chrono::milliseconds( 20 );

    parallel(
        // channel duration timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_for( "movable", duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_for.move_tx() ),
        proc( [duration,delay]( auto rx ) {
                std::string item;
                auto res = rx.recv_for( item, duration );
                this_proc::delay_for( delay );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for( delay );
                res = rx.recv_for( item, duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_for.move_rx() ),

        // channel time_point timeout
        proc( [duration]( auto tx ) {
                std::string item = "const ref";
                auto res = tx.send_until( item, std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
                res = tx.send_until( "movable", std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_until.move_tx() ),
        proc( [duration,delay]( auto rx ) {
                std::string item;
                this_proc::delay_for( delay );
                auto res = rx.recv_until( item, std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );

                this_proc::delay_for( delay );
                res = rx.recv_until( item, std::chrono::steady_clock::now() + duration );
                throw_assert( res == OpResult::Ok, "OpResult should be Ok" );
            }, ch_until.move_rx() )
    );
}

int main()
{
    test_item_copy_move();
    test_item_copy_move_timeout();
    test_channel_sync_works();
    test_range_recv();
    test_timeout_one_side();
    test_timeout_complete();
    test_timeout_tx_delayed();
    test_timeout_rx_delayed();

    return 0;
}
