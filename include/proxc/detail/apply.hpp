
#pragma once

#include <functional>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#if defined(PROXC_COMP_MSVC)
#   pragma warning(push)
#   pragma warning(disable: 4100)
#endif

PROXC_NAMESPACE_BEGIN
namespace detail {

template< typename Fn
        , typename ... Args
>
auto invoke( Fn && fn, Args && ... args )
    -> std::enable_if_t<
           std::is_member_pointer<
               typename std::decay< Fn >::type
           >::value,
           typename std::result_of< Fn && ( Args && ... ) >
       >
{
    return std::mem_fn( fn )( std::forward< Args >( args ) ... );
}

template< typename Fn
        , typename ... Args
>
auto invoke( Fn && fn, Args && ... args )
    -> std::enable_if_t<
           ! std::is_member_pointer<
               typename std::decay< Fn >::type
           >::value,
           typename std::result_of_t< Fn && ( Args && ... ) >
       >
{
    return std::forward< Fn >( fn )( std::forward< Args >( args ) ... );
}

template< typename Fn
        , typename Tpl
        , std::size_t ... I
>
auto apply_impl( Fn && fn, Tpl && tpl, std::index_sequence< I ... > )
    -> decltype( invoke( std::forward< Fn >( fn ),
                         std::get< I >( std::forward< Tpl >( tpl ) ) ... ) )
{
    return invoke( std::forward< Fn >( fn ),
                   std::get< I >( std::forward< Tpl >( tpl ) ) ...  );
}

template< typename Fn
        , typename Tpl
>
auto apply( Fn && fn, Tpl && tpl )
    -> decltype( apply_impl(
            std::forward< Fn >( fn ),
            std::forward< Tpl >( tpl ),
            std::make_index_sequence<
                std::tuple_size<
                    typename std::decay< Tpl >::type
                >::value
            >{} ) )
{
    return apply_impl(
        std::forward< Fn >( fn ),
        std::forward< Tpl >( tpl ),
        std::make_index_sequence<
            std::tuple_size<
                typename std::decay< Tpl >::type
            >::value
        >{} );
}

} // namespace detail
PROXC_NAMESPACE_END

#if defined(PROXC_COMP_MSVC)
#   pragma warning(pop)
#endif

