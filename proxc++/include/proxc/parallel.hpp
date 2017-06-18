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
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/range/irange.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename P>
struct is_proc
    : std::integral_constant<
        bool,
        std::is_same<
            typename std::decay< P >::type,
            Process
        >::value
    >
{};

template<typename ...>
struct are_procs
    : std::true_type
{};

template<typename Proc, typename ... Procs>
struct are_procs< Proc, Procs ... >
    : std::integral_constant<
        bool,
           is_proc< Proc >::value
        && are_procs< Procs ... >{}
    >
{};

template<typename Proc>
auto parallel_impl( Proc && proc )
    -> std::enable_if_t<
           is_proc< Proc >::value
        && std::is_move_assignable< Proc >::value
    >
{
    Proc p = std::forward< Proc >( proc );
    p.launch();
    p.join();
}

template<typename Proc, typename ... Procs>
auto parallel_impl(Proc && proc, Procs && ... procs )
    -> std::enable_if_t<
           is_proc< Proc >::value
        && std::is_move_assignable< Proc >::value
    >
{
    Proc p = std::forward< Proc >( proc );
    p.launch();
    parallel_impl( std::forward< Procs >( procs ) ... );
    p.join();
}

} // namespace detail

template<typename ... Procs>
void parallel(Procs && ... procs)
{
    static_assert( detail::are_procs< Procs ... >::value,
        "Supplied arguments must be of type proxc::Process" );
    static_assert( detail::traits::are_move_assigneable< Procs ... >::value,
        "Supplied processes must be move assigneable" );
    detail::parallel_impl( std::forward< Procs >( procs ) ... );
}

PROXC_NAMESPACE_END

