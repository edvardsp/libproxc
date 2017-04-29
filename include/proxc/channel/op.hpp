
#pragma once

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
PROXC_NAMESPACE_END

