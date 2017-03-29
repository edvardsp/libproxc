
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

template<typename T>
typename std::decay< T >::type
decay_copy(T && t)
{
    return std::forward< T >(t);
}

} // namespace traits

PROXC_NAMESPACE_END


