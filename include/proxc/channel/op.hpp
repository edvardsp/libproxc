
#pragma once

#include <tuple>
#include <type_traits>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {

template<typename Tx, typename Rx>
auto get_tx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        std::is_trivially_copyable< Tx >::value,
        Tx
    >
{
    return std::get< 0 >( tpl );
}

template<typename Tx, typename Rx>
auto get_tx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        ! std::is_trivially_copyable< Tx >::value,
        Tx
    >
{
    return std::move( std::get< 0 >( tpl ) );
}

template<typename Tx, typename Rx>
auto get_rx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        std::is_trivially_copyable< Rx >::value,
        Rx
    >
{
    return std::get< 1 >( tpl );
}

template<typename Tx, typename Rx>
auto get_rx( std::tuple< Tx, Rx > & tpl )
    -> std::enable_if_t<
        ! std::is_trivially_copyable< Rx >::value,
        Rx
    >
{
    return std::move( std::get< 1 >( tpl ) );
}

} // namespace channel
PROXC_NAMESPACE_END

