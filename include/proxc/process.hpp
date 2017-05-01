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

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>
#include <proxc/detail/apply.hpp>
#include <proxc/detail/delegate.hpp>
#include <proxc/detail/traits.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/range/irange.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process definition
////////////////////////////////////////////////////////////////////////////////

class Process
{
private:
    using CtxPtr = boost::intrusive_ptr< runtime::Context >;

    CtxPtr    ctx_{};

public:
    using Id = runtime::Context::Id;

    Process() = default;

    template<typename Fn,
             typename ... Args
    >
    Process( Fn && fn, Args && ... args );

    ~Process() noexcept;

    // make non-copyable
    Process( Process const & ) = delete;
    Process & operator = ( Process const & ) = delete;

    // make moveable
    Process( Process && ) noexcept;
    Process & operator = ( Process && ) noexcept;

    void swap( Process & other ) noexcept;
    Id get_id() const noexcept;

    void launch() noexcept;
    void join();
    void detach();
};

////////////////////////////////////////////////////////////////////////////////
// Process implementation
////////////////////////////////////////////////////////////////////////////////

template< typename Fn,
          typename ... Args
>
Process::Process( Fn && fn, Args && ... args )
    : ctx_{ runtime::Scheduler::make_work(
        std::forward< Fn >( fn ),
        std::forward< Args >( args ) ...
    ) }
{
}

////////////////////////////////////////////////////////////////////////////////
// Process methods
////////////////////////////////////////////////////////////////////////////////

template<typename Fn, typename ... Args>
Process proc( Fn &&, Args && ... );

namespace detail {

template< typename InputIt,
          typename ... Fns
>
auto proc_for_impl( InputIt first, InputIt last, Fns && ... fns )
    -> std::enable_if_t<
        detail::traits::is_inputiterator< InputIt >{}
    >
{
    static_assert( detail::traits::are_callable_with_arg< typename InputIt::value_type, Fns ... >{},
        "Supplied functions does not have the correct function signature" );

    using FnT = detail::delegate< void( typename InputIt::value_type ) >;
    using ArrT = std::array< FnT, sizeof...(Fns) >;

    auto fn_arr = ArrT{ { std::forward< Fns >( fns ) ... } };
    const std::size_t total_size = sizeof...(Fns) * std::distance( first, last );
    std::vector< Process > procs;
    procs.reserve( total_size );

    for ( auto data = first; data != last; ++data ) {
        for ( const auto& fn : fn_arr ) {
            procs.emplace_back( fn, *data );
        }
    }
    std::for_each( procs.begin(), procs.end(),
        []( auto& proc ){ proc.launch(); } );
    std::for_each( procs.begin(), procs.end(),
        []( auto& proc ){ proc.join(); } );
}

template< typename Input,
          typename ... Fns
>
auto proc_for_impl( Input first, Input last, Fns && ... fns )
    -> std::enable_if_t<
        std::is_integral< Input >{}
    >
{
    auto iota = boost::irange( first, last );
    proc_for_impl( iota.begin(), iota.end(), std::forward< Fns >( fns ) ... );
}

} // namespace detail

template< typename Fn,
          typename ... Args
>
Process proc( Fn && fn, Args && ... args )
{
    return Process{
        std::forward< Fn >( fn ),
        std::forward< Args >( args ) ...
    };
}

template< typename Input,
          typename ... Fns
>
Process proc_for( Input first, Input last, Fns && ... fns )
{
    return Process{
        & detail::proc_for_impl< Input, Fns ... >,
        first, last,
        std::forward< Fns >( fns ) ...
    };
}

template< typename InputIt
        , typename = std::enable_if_t<
            std::is_same<
                Process,
                typename std::decay<
                    typename std::iterator_traits< InputIt>::value_type
                >::type
            >::value
        > >
Process proc_for( InputIt first, InputIt last )
{
    return Process{
        [first,last]{
            std::for_each( first, last,
                []( auto& proc ){ proc.launch(); } );
            std::for_each( first, last,
                []( auto& proc ){ proc.join(); } );
        }
    };
}

PROXC_NAMESPACE_END

