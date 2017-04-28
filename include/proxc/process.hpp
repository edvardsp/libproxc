
#pragma once

#include <functional>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/traits.hpp>
#include <proxc/detail/apply.hpp>
#include <proxc/detail/delegate.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/range/irange.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process definition
////////////////////////////////////////////////////////////////////////////////

class Process
{
private:
    using CtxPtr = boost::intrusive_ptr< Context >;

    CtxPtr    ctx_{};

public:
    using Id = Context::Id;

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
    : ctx_{ Scheduler::make_work(
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
        traits::is_inputiterator< InputIt >{}
    >
{
    static_assert( traits::are_callable_with_arg< typename InputIt::value_type, Fns ... >{},
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

PROXC_NAMESPACE_END

