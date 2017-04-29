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

