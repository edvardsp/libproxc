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

#include <proxc/config.hpp>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>

PROXC_NAMESPACE_BEGIN

namespace detail {
namespace queue {

namespace intrusive = boost::intrusive;

template<typename T, typename Hook, Hook T::* Ptr>
using MemberHook = intrusive::member_hook<
    T, Hook, Ptr 
>;

template<typename T, typename Hook, Hook T::* Ptr>
using ListQueue = intrusive::list<
    T,
    MemberHook< T, Hook, Ptr >,
    intrusive::constant_time_size< false > >;

template<typename T, typename Hook, Hook T::* Ptr, typename Fn>
using SetQueue = intrusive::multiset<
    T,
    MemberHook< T, Hook, Ptr >,
    intrusive::constant_time_size< false >,
    intrusive::compare< Fn > >;

} // namespace queue
} // namespace detail

PROXC_NAMESPACE_END

