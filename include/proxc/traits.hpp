
#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN

namespace traits {

namespace detail {

template<typename T>
using always_void = void;

template<typename Expr, typename Enable = void>
struct is_callable_impl
    : std::false_type
{};

template<typename Fn, typename ... Args>
struct is_callable_impl< Fn(Args ...), always_void< std::result_of_t< Fn(Args ...) > > >
    : std::true_type
{};

} // namespace detail

template<typename Expr>
struct is_callable
    : detail::is_callable_impl< Expr >
{};

template<typename Arg, typename ...>
struct are_callable_with_arg
    : std::true_type
{};

template<typename Arg, typename Fn, typename ... Fns>
struct are_callable_with_arg< Arg, Fn, Fns ... >
    : std::integral_constant< bool,
           is_callable< Fn(Arg) >{}
        && are_callable_with_arg< Arg, Fns ... >{}
    >
{};

template<typename ...>
struct are_move_assigneable
    : std::true_type
{};

template<typename Proc, typename ... Procs>
struct are_move_assigneable< Proc, Procs ... >
    : std::integral_constant< bool,
           std::is_move_assignable< Proc >::value
        && are_move_assigneable< Procs ... >{}
    >
{};

template<typename It, typename = void>
struct is_inputiterator
    : std::false_type
{};

template<typename It>
struct is_inputiterator< It, decltype( *std::declval< It & >(), ++std::declval< It & >(), void() )>
    : std::true_type
{};


template<typename T>
typename std::decay< T >::type
decay_copy(T && t)
{
    return std::forward< T >(t);
}

} // namespace traits

PROXC_NAMESPACE_END


