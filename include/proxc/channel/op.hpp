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
using TxVec = std::vector< Tx< T > >;
template<typename T>
using RxVec = std::vector< Rx< T > >;

template<typename T>
Tx< T > get_tx( std::tuple< Tx< T >, Rx< T > > & tpl )
{
    return std::move( std::get< 0 >( tpl ) );
}

template<typename T>
TxVec< T > get_tx( std::tuple< TxVec< T >, RxVec< T > > & tpl )
{
    return std::move( std::get< 0 >( tpl ) );
}

template<typename T>
Tx< T > get_tx_ind( TxVec< T > & tx_vec, std::size_t ind )
{
    return std::move( tx_vec[ ind ] );
}

template<typename T>
Tx< T > get_tx_ind( std::tuple< TxVec< T >, RxVec< T > > & tpl, std::size_t ind )
{
    return std::move( std::get< 0 >( tpl )[ ind ] );
}

template<typename T>
Rx< T > get_rx( std::tuple< Tx< T >, Rx< T > > & tpl )
{
    return std::move( std::get< 1 >( tpl ) );
}

template<typename T>
RxVec< T > get_rx( std::tuple< TxVec< T >, RxVec< T > > & tpl )
{
    return std::move( std::get< 1 >( tpl ) );
}

template<typename T>
Rx< T > get_rx_ind( RxVec< T > & rx_vec, std::size_t ind )
{
    return std::move( rx_vec[ ind ] );
}

template<typename T>
Rx< T > get_rx_ind( std::tuple< TxVec< T >, RxVec< T > > & tpl, std::size_t ind )
{
    return std::move( std::get< 1 >( tpl )[ ind ] );
}

template<typename T>
std::tuple< Tx< T >, Rx< T > > create() noexcept
{
    auto channel = std::make_shared< detail::ChannelImpl< T > >();
    return std::make_tuple( Tx< T >{ channel }, Rx< T >{ channel } );
}

template<typename T>
std::tuple< TxVec< T >, RxVec< T > >
create_n( const std::size_t n ) noexcept
{
    std::vector< Tx< T > > txs;
    std::vector< Rx< T > > rxs;
    txs.reserve( n );
    rxs.reserve( n );
    Tx< T > tx;
    Rx< T > rx;
    for( std::size_t i = 0; i < n; ++i ) {
        std::tie( tx, rx ) = create< T >();
        txs.push_back( std::move( tx ) );
        rxs.push_back( std::move( rx ) );
    }
    return std::make_tuple( std::move( txs ), std::move( rxs ) );
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
        VecTxT txs;
        txs.reserve( this->size() );
        std::for_each( this->begin(), this->end(),
            [&txs]( auto& ch ){ txs.push_back( std::move( ch.move_tx() ) ); } );
        return std::move( txs );
    }

    VecRxT collect_rx() noexcept
    {
        VecRxT rxs;
        rxs.reserve( this->size() );
        std::for_each( this->begin(), this->end(),
            [&rxs]( auto& ch ){ rxs.push_back( std::move( ch.move_rx() ) ); } );
        return std::move( rxs );
    }
};


PROXC_NAMESPACE_END

