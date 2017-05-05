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
#include <memory>
#include <tuple>
#include <type_traits>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>
#include <proxc/channel/sync.hpp>

PROXC_NAMESPACE_BEGIN

// forward declarations
class Alt;

namespace alt {

template<typename T>
class ChoiceRecv;

} // namespace alt

namespace channel {

// forward declaration
template<typename T> class Tx;

////////////////////////////////////////////////////////////////////////////////
// Rx
////////////////////////////////////////////////////////////////////////////////

namespace detail {

struct RxBase {};

} // namespace detail

template<typename T>
class Rx : public detail::RxBase
{
public:
    using ItemT = typename std::decay< T >::type;
    using Id = detail::ChannelId;

private:
    using ChanT = detail::ChannelImpl< ItemT >;
    using EndT  = detail::ChanEnd< ItemT >;
    using ChanPtr = std::shared_ptr< ChanT >;

    ChanPtr    chan_{ nullptr };

public:
    Rx() = default;
    ~Rx()
    {
        close();
    }

    // make non-copyable
    Rx( Rx const & )               = delete;
    Rx & operator = ( Rx const & ) = delete;

    // make movable
    Rx( Rx && )               = default;
    Rx & operator = ( Rx && ) = default;

    Id get_id() const noexcept
    {
        return Id{ chan_.get() };
    }

    bool is_closed() const noexcept
    {
        return chan_->is_closed();
    }

    void close() noexcept
    {
        if ( chan_ ) {
            chan_->close();
            chan_.reset();
        }
    }

    // normal recv operations
    OpResult recv( ItemT & item ) noexcept
    {
        EndT rx{ runtime::Scheduler::running(), item };
        return chan_->recv( rx );
    }

    // recv operation with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult recv_until( ItemT & item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        EndT rx{ runtime::Scheduler::running(), item };
        return chan_->recv_until( rx, time_point );
    }

    // recv operation with duration timeout
    template<typename Rep, typename Period>
    OpResult recv_for( ItemT & item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return recv_until( item, time_point );
    }

    explicit operator bool() const noexcept
    {
        return ! is_closed();
    }

    bool operator ! () const noexcept
    {
        return is_closed();
    }

    template<typename ItemU>
    friend bool operator >> ( Rx< ItemU > & rx, ItemU & item )
    {
        return rx.recv( item ) == OpResult::Ok;
    }

private:
    Rx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

    friend class ::proxc::alt::ChoiceRecv< ItemT >;

    void alt_enter( EndT & rx ) noexcept
    {
        chan_->alt_recv_enter( rx );
    }

    void alt_leave() noexcept
    {
        chan_->alt_recv_leave();
    }

    bool alt_ready() const noexcept
    {
        return chan_->alt_recv_ready();
    }

    AltResult alt_recv() noexcept
    {
        return chan_->alt_recv();
    }

public:
    class Iterator
        : public std::iterator<
            std::input_iterator_tag,
            typename std::remove_reference< ItemT >::type
          >
    {
    private:
        using StorageT = typename std::aligned_storage<
            sizeof( ItemT ), alignof( ItemT )
        >::type;
        using RxT = Rx< ItemT >;

        RxT *       rx_{ nullptr };
        StorageT    storage_;

        void increment() noexcept
        {
            BOOST_ASSERT( rx_ != nullptr );
            ItemT item{};
            auto res = rx_->recv( item );
            if ( res == OpResult::Ok ) {
                auto addr = static_cast< void * >( std::addressof( storage_ ) );
                ::new (addr) ItemT{ std::move( item ) };
            } else {
                rx_ = nullptr;
            }
        }

    public:
        using Ptr = typename Iterator::pointer;
        using Ref = typename Iterator::reference;

        Iterator() noexcept = default;

        explicit Iterator( Rx< ItemT > * rx ) noexcept
            : rx_{ rx }
        { increment(); }

        Iterator( Iterator const & other ) noexcept
            : rx_{ other.rx_ }
        {}

        Iterator & operator = ( Iterator const & other ) noexcept
        {
            if ( this == & other ) {
                return *this;
            }
            rx_ = other.rx_;
            return *this;
        }

        bool operator == ( Iterator const & other ) const noexcept
        { return rx_ == other.rx_; }

        bool operator != ( Iterator const & other ) const noexcept
        { return rx_ != other.rx_; }

        Iterator & operator ++ () noexcept
        {
            increment();
            return *this;
        }

        Iterator operator ++ ( int ) = delete;

        Ref operator * () noexcept
        { return * reinterpret_cast< ItemT * >( std::addressof( storage_ ) ); }

        Ptr operator -> () noexcept
        { return reinterpret_cast< ItemT * >( std::addressof( storage_ ) ); }
    };
};

template<typename ItemT>
typename Rx< ItemT >::Iterator
begin( Rx< ItemT > & chan )
{
    return typename Rx< ItemT >::Iterator( & chan );
}

template<typename ItemT>
typename Rx< ItemT >::Iterator
end( Rx< ItemT > & )
{
    return typename Rx< ItemT >::Iterator();
}

} // namespace channel

namespace detail {
namespace traits {

template<typename Rx>
struct is_rx
    : std::integral_constant<
        bool,
        std::is_base_of<
            channel::detail::RxBase,
            Rx
        >::value
    >
{};

template<typename RxIt>
struct is_rx_iterator
    : std::integral_constant<
        bool,
        is_rx<
            typename std::iterator_traits< RxIt >::value_type
        >::value
    >
{};

} // namespace traits
} // namespace detail
PROXC_NAMESPACE_END

