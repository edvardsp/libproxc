
#pragma once

#include <chrono>
#include <iterator>
#include <memory>
#include <type_traits>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/channel/sync.hpp>

PROXC_NAMESPACE_BEGIN

// forward declaration
class Alt;

namespace alt {

template<typename T> class ChoiceSend;

} // namespace alt

namespace channel {

// forward declaration
template<typename T> class Rx;

////////////////////////////////////////////////////////////////////////////////
// Tx
////////////////////////////////////////////////////////////////////////////////

namespace detail {

struct TxBase {};

} // namespace detail

template<typename T>
class Tx : public detail::TxBase
{
public:
    using ItemT = typename std::decay< T >::type;
    using Id = detail::ChannelId;

private:
    using ChanT = detail::ChannelImpl< ItemT >;
    using EndT = detail::ChanEnd< ItemT >;
    using ChanPtr = std::shared_ptr< ChanT >;

    ChanPtr    chan_{ nullptr };

public:
    Tx() = default;
    ~Tx()
    {
        if ( chan_ ) {
            chan_->close();
        }
    }

    // make non-copyable
    Tx( Tx const & )               = delete;
    Tx & operator = ( Tx const & ) = delete;

    // make movable
    Tx( Tx && ) = default;
    Tx & operator = ( Tx && ) = default;

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
        chan_->close();
        chan_.reset();
    }

    // normal send operations
    OpResult send( ItemT const & item ) noexcept
    {
        ItemT i{ item };
        EndT tx{ Scheduler::running(), i };
        return chan_->send( tx );
    }

    OpResult send( ItemT && item ) noexcept
    {
        ItemT i{ std::move( item ) };
        EndT tx{ Scheduler::running(), i };
        return chan_->send( tx );
    }

    // send operations with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( ItemT && item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        EndT tx{ Scheduler::running(), item };
        return chan_->send_until( tx, time_point );
    }

    template<typename Clock, typename Dur>
    OpResult send_until( ItemT const & item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        ItemT i{ item };
        EndT tx{ Scheduler::running(), i };
        return chan_->send_until( tx, time_point );
    }

    // send operations with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( ItemT && item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return send_until( std::move( item ), time_point );
    }

    template<typename Rep, typename Period>
    OpResult send_for( ItemT const & item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return send_until( item, time_point );
    }


private:
    Tx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

    friend class ::proxc::alt::ChoiceSend< ItemT >;

    void alt_enter( EndT & tx ) noexcept
    {
        chan_->alt_send_enter( tx );
    }

    void alt_leave() noexcept
    {
        chan_->alt_send_leave();
    }

    bool alt_ready() const noexcept
    {
        return chan_->alt_send_ready();
    }

    AltResult alt_send() noexcept
    {
        return chan_->alt_send();
    }
};

} // namespace channel

namespace traits {

template<typename Tx>
struct is_tx
    : std::integral_constant<
        bool,
        std::is_base_of<
            channel::detail::TxBase,
            Tx
        >::value
    >
{};

template<typename TxIt>
struct is_tx_iterator
    : std::integral_constant<
        bool,
        is_tx<
            typename std::iterator_traits< TxIt >::value_type
        >::value
    >
{};

} // namespace traits
PROXC_NAMESPACE_END

