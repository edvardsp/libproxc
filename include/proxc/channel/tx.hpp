
#pragma once

#include <chrono>
#include <memory>

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

template<typename T>
class Tx
{
public:
    using ItemType = T;

private:
    using ChanType = detail::ChannelImpl< ItemType >;
    using ChanPtr = std::shared_ptr< ChanType >;

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
    OpResult send( ItemType const & item ) noexcept
    {
        ChanEnd tx{ Scheduler::running() };
        ItemType i{ item };
        return chan_->send( tx, i );
    }

    OpResult send( ItemType && item ) noexcept
    {
        ChanEnd tx{ Scheduler::running() };
        ItemType i{ std::move( item ) };
        return chan_->send( tx, i );
    }

    // send operations with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( ItemType && item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        ChanEnd tx{ Scheduler::running() };
        return chan_->send_until( tx, item, time_point );
    }

    template<typename Clock, typename Dur>
    OpResult send_until( ItemType const & item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        ChanEnd tx{ Scheduler::running() };
        ItemType i{ item };
        return chan_->send_until( tx, i, time_point );
    }

    // send operations with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( ItemType && item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return send_until( std::move( item ), time_point );
    }

    template<typename Rep, typename Period>
    OpResult send_for( ItemType const & item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        ItemType i{ item };
        return send_until( std::move( i ), time_point );
    }


private:
    Tx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

    friend class ::proxc::alt::ChoiceSend< T >;

    void alt_enter( ChanEnd & tx ) noexcept
    {
        chan_->alt_send_enter( tx );
    }

    void alt_leave() noexcept
    {
        chan_->alt_send_leave();
    }

    bool alt_ready( Alt * alt ) const noexcept
    {
        return chan_->alt_send_ready( alt );
    }

    AltResult alt_send( ItemType & item ) noexcept
    {
        return chan_->alt_send( item );
    }
};

} // namespace channel
PROXC_NAMESPACE_END

