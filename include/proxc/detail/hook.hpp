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
namespace hook {

namespace intrusive = boost::intrusive;

template<typename Tag>
struct ListHook
{
private:
    using MemberHook = intrusive::list_member_hook<
        intrusive::tag< Tag >,
        intrusive::link_mode< intrusive::auto_unlink >
    >;
public:
    using Type = MemberHook;
};

template<typename Tag>
struct SetHook
{
private:
    using MemberHook = intrusive::set_member_hook<
        intrusive::tag< Tag >,
        intrusive::link_mode< intrusive::auto_unlink >
    >;
public:
    using Type = MemberHook;
};

struct ReadyTag;
struct WorkTag;
struct WaitTag;
struct SleepTag;
struct TerminatedTag;

using Ready      = ListHook< ReadyTag >::Type;
using Work       = ListHook< WorkTag >::Type;
using Wait       = ListHook< WaitTag >::Type;
using Sleep      = SetHook< SleepTag >::Type;
using Terminated = ListHook< TerminatedTag >::Type;

} // namespace hook
} // namespace detail
PROXC_NAMESPACE_END

