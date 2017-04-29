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


