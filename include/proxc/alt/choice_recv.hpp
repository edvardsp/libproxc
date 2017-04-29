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

#include <proxc/channel.hpp>
#include <proxc/context.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/detail/delegate.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceRecv : public ChoiceBase
{
public:
    using ItemT = T;
    using RxT   = channel::Rx< ItemT >;
    using EndT  = channel::detail::ChanEnd< ItemT >;
    using FnT   = proxc::detail::delegate< void( ItemT ) >;

private:
    RxT &    rx_;
    ItemT    item_;
    EndT     end_;
    FnT      fn_;

public:
    ChoiceRecv( Alt *     alt,
                Context * ctx,
                RxT &     rx,
                FnT       fn )
        : ChoiceBase{ alt }
        , rx_{ rx }
        , item_{}
        , end_{ ctx, item_, this }
        , fn_{ std::move( fn ) }
    {}

    ~ChoiceRecv() {}

    void enter() noexcept
    {
        rx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        rx_.alt_leave();
    }

    bool is_ready() const noexcept
    {
        return rx_.alt_ready();
    }

    Result try_complete() noexcept
    {
        auto res = rx_.alt_recv();
        switch ( res ) {
        case channel::AltResult::Ok:       return Result::Ok;
        case channel::AltResult::TryLater: return Result::TryLater;
        default:                           return Result::Failed;
        }
    }

    void run_func() const noexcept
    {
        if ( fn_ ) {
            fn_( std::move( item_ ) );
        }
    }
};

} // namespace alt
PROXC_NAMESPACE_END


