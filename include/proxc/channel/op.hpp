
#pragma once

#include <tuple>
#include <type_traits>

#include <proxc/config.hpp>

#include <proxc/channel/base.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {
namespace detail {

template<typename Ch>
struct is_channel
{
    enum {
        value = std::is_base_of< ChannelBase, Ch >::value,
    };
};

template<typename Tx>
struct is_channel_tx 
{
    enum {
        value = std::is_base_of< TxBase, Tx >::value,
    };
};

template<typename Rx>
struct is_channel_rx
{
    enum {
        value = std::is_base_of< RxBase, Rx >::value,
    };
};

} // detail

template<typename Tx, typename Rx>
auto get_tx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        std::is_trivially_copyable< Tx >::value,
        Tx
    >
{
    static_assert( detail::is_channel_tx< Tx >::value, "supplied Tx is not a valid Channel Tx" );
    static_assert( detail::is_channel_rx< Rx >::value, "supplied Rx is not a valid Channel Rx" );
    return std::get< 0 >( tpl );
}

template<typename Tx, typename Rx>
auto get_tx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        ! std::is_trivially_copyable< Tx >::value,
        Tx
    >
{
    static_assert( detail::is_channel_tx< Tx >::value, "supplied Tx is not a valid Channel Tx" );
    static_assert( detail::is_channel_rx< Rx >::value, "supplied Rx is not a valid Channel Rx" );
    return std::move( std::get< 0 >( tpl ) );
}

template<typename Tx, typename Rx>
auto get_rx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        std::is_trivially_copyable< Rx >::value,
        Rx
    >
{
    static_assert( detail::is_channel_tx< Tx >::value, "supplied Tx is not a valid Channel Tx" );
    static_assert( detail::is_channel_rx< Rx >::value, "supplied Rx is not a valid Channel Rx" );
    return std::get< 1 >( tpl );
}

template<typename Tx, typename Rx>
auto get_rx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        ! std::is_trivially_copyable< Rx >::value,
        Rx
    >
{
    static_assert( detail::is_channel_tx< Tx >::value, "supplied Tx is not a valid Channel Tx" );
    static_assert( detail::is_channel_rx< Rx >::value, "supplied Rx is not a valid Channel Rx" );
    return std::move( std::get< 1 >( tpl ) );
}

} // namespace channel
PROXC_NAMESPACE_END

