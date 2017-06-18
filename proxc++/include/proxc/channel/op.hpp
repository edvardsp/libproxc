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

#pragma once

#include <array>
#include <memory>
#include <tuple>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/channel/sync.hpp>
#include <proxc/channel/tx.hpp>
#include <proxc/channel/rx.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {

template<typename T>
std::tuple< Tx< T >, Rx< T > > create() noexcept
{
    auto channel = std::make_shared< detail::ChannelImpl< T > >();
    return std::make_tuple( Tx< T >{ channel }, Rx< T >{ channel } );
}

} // namespace channel

template<typename T>
struct Chan : public std::tuple< channel::Tx< T >, channel::Rx< T > >
{
    using Tx = channel::Tx< T >;
    using Rx = channel::Rx< T >;

    using TplT = std::tuple< Tx, Rx >;
    using TplT::TplT;

    Chan() : TplT{ channel::create< T >() }
    {}

    // make non-copyable
    Chan( Chan const & )               = delete;
    Chan & operator = ( Chan const & ) = delete;

    // make moveable
    Chan( Chan && )               = default;
    Chan & operator = ( Chan && ) = default;


    Tx & ref_tx() noexcept
    {
        return std::get< 0 >( *this );
    }

    Rx & ref_rx() noexcept
    {
        return std::get< 1 >( *this );
    }

    Tx move_tx() noexcept
    {
        return std::move( std::get< 0 >( *this ) );
    }

    Rx move_rx() noexcept
    {
        return std::move( std::get< 1 >( *this ) );
    }
};

template<typename T, std::size_t N>
struct ChanArr : public std::array< Chan< T >, N >
{
    using Tx = typename Chan< T >::Tx;
    using Rx = typename Chan< T >::Rx;

    using ArrT = std::array< Chan< T >, N >;
    using ArrT::ArrT;

    using ArrTxT = std::array< Tx, N >;
    using ArrRxT = std::array< Rx, N >;

    ChanArr() : ArrT()
    {}

    ArrTxT collect_tx() noexcept
    {
        ArrTxT txs;
        auto ch_it = this->begin();
        std::generate( txs.begin(), txs.end(),
            [&ch_it]{ return (ch_it++)->move_tx(); } );
        return std::move( txs );
    }

    ArrRxT collect_rx() noexcept
    {
        ArrRxT rxs;
        auto ch_it = this->begin();
        std::generate( rxs.begin(), rxs.end(),
            [&ch_it]{ return (ch_it++)->move_rx(); } );
        return std::move( rxs );
    }
};

template<typename T>
struct ChanVec : public std::vector< Chan< T > >
{
    using Tx = typename Chan< T >::Tx;
    using Rx = typename Chan< T >::Rx;

    using VecT = std::vector< Chan< T > >;
    using VecT::VecT;

    using VecTxT = std::vector< Tx >;
    using VecRxT = std::vector< Rx >;

    ChanVec() = delete;
    ChanVec( std::size_t n ) : VecT( n )
    {}

    VecTxT collect_tx() noexcept
    {
        VecTxT txs( this->size() );
        auto ch_it = this->begin();
        std::generate( txs.begin(), txs.end(),
            [&ch_it]{ return (ch_it++)->move_tx(); } );
        return std::move( txs );
    }

    VecRxT collect_rx() noexcept
    {
        VecRxT rxs;
        auto ch_it = this->begin();
        std::generate( rxs.begin(), rxs.end(),
            [&ch_it]{ return (ch_it++)->move_rx(); } );
        return std::move( rxs );
    }
};


PROXC_NAMESPACE_END

