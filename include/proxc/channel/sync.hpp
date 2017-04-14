
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/channel/op_result.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

// forward declaration
class Alt;

namespace channel {

struct alignas(cache_alignment) ChanEnd
{
    Context *     ctx_;
    Alt *         alt_;
    ChanEnd( Context * ctx, Alt * alt = nullptr )
        : ctx_{ ctx }, alt_{ alt }
    {}
};

namespace detail {

////////////////////////////////////////////////////////////////////////////////
// ChannelImpl
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class ChannelImpl
{
public:
    using ItemType = T;

private:
    struct alignas(cache_alignment) Payload
    {
        ItemType &    item_;
        Payload( ItemType & item )
            : item_{ item }
        {}
    };

    Spinlock    splk_{};

    alignas(cache_alignment) std::atomic< bool >         closed_{ false };

    alignas(cache_alignment) std::atomic< ChanEnd * >    tx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< ChanEnd * >    rx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< Payload * >    payload_{ nullptr };

    // used to synchronize during alting, on either Tx or Rx end
    alignas(cache_alignment) std::atomic< bool >    alt_sync_{ false };

public:
    ChannelImpl() = default;
    ~ChannelImpl() noexcept;

    // make non-copyable
    ChannelImpl( ChannelImpl const & )               = delete;
    ChannelImpl & operator = ( ChannelImpl const & ) = delete;

    bool is_closed() const noexcept;
    void close() noexcept;

    bool has_tx_() const noexcept;
    bool has_rx_() const noexcept;

    // normal send and receieve methods
    OpResult send( ChanEnd &, ItemType & ) noexcept;

    OpResult recv( ChanEnd &, ItemType & ) noexcept;

    // send and receive methods with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( ChanEnd &,
                       ItemType &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;
    template<typename Rep, typename Period>
    OpResult recv_for( ChanEnd &,
                       ItemType &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;

    // send and receive methods with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( ChanEnd &,
                         ItemType &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;
    template<typename Clock, typename Dur>
    OpResult recv_until( ChanEnd &,
                         ItemType &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;

    // send and receive methods for alting
    void alt_send_enter( ChanEnd & ) noexcept;
    void alt_send_leave() noexcept;
    bool alt_send_ready( Alt * ) const noexcept;
    AltResult alt_send( ItemType & ) noexcept;

    void alt_recv_enter( ChanEnd & ) noexcept;
    void alt_recv_leave() noexcept;
    bool alt_recv_ready( Alt * ) const noexcept;
    AltResult alt_recv( ItemType & ) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// ChannelImpl public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
ChannelImpl< T >::~ChannelImpl< T >() noexcept
{
    close();

    BOOST_ASSERT( ! has_tx_() );
    BOOST_ASSERT( ! has_rx_() );
}

template<typename T>
bool ChannelImpl< T >::is_closed() const noexcept
{
    return closed_.load( std::memory_order_acquire );
}

template<typename T>
void ChannelImpl< T >::close() noexcept
{
    closed_.store( true, std::memory_order_release );
    auto tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( tx != nullptr ) {
        // FIXME: call item destructor?
        if ( tx->alt_ != nullptr ) {
            // FIXME
            /* tx->alt_->maybe_wakeup(); */
        } else {
            BOOST_ASSERT( tx->ctx_ != nullptr );
            Scheduler::self()->schedule( tx->ctx_ );
        }
    }
    auto rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        if ( rx->alt_ != nullptr ) {
            // FIXME
            /* rx->alt_->maybe_wakeup(); */
        } else {
            BOOST_ASSERT( rx->ctx_ != nullptr );
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }
}

template<typename T>
bool ChannelImpl< T >::has_tx_() const noexcept
{
    return tx_end_.load( std::memory_order_relaxed ) != nullptr;
}

template<typename T>
bool ChannelImpl< T >::has_rx_() const noexcept
{
    return rx_end_.load( std::memory_order_relaxed ) != nullptr;
}

template<typename T>
OpResult ChannelImpl< T >::send( ChanEnd & tx, ItemType & item ) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    tx_end_.store( std::addressof( tx ), std::memory_order_release );
    Payload payload{ item };
    payload_.store( std::addressof( payload ), std::memory_order_release );

    auto rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        if ( rx->alt_ != nullptr ) {
            // FIXME
            /* rx->alt_->maybe_wakeup(); */
        } else {
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }

    Scheduler::self()->wait( & lk );
    // Rx has consumed item
    return OpResult::Ok;
}

template<typename T>
OpResult ChannelImpl< T >::recv( ChanEnd & rx, ItemType & item ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        auto tx = tx_end_.load( std::memory_order_acquire );
        if ( tx != nullptr ) {
            if ( tx->alt_ && ! alt_sync_.load( std::memory_order_acquire ) ) {
                // order matters here
                rx_end_.store( std::addressof( rx ), std::memory_order_release );
                // FIXME
                /* tx->alt_->maybe_wakeup(); */

                Scheduler::self()->wait( & lk, true );

                if ( ! alt_sync_.load( std::memory_order_acquire ) ) {
                    continue;
                }
                // alting Tx synced, can now consume item
            }

            tx_end_.store( nullptr, std::memory_order_release );
            auto payload = payload_.exchange( nullptr, std::memory_order_acq_rel );
            BOOST_ASSERT( payload != nullptr );
            // order matters here
            item = std::move( payload->item_ );
            Scheduler::self()->schedule( tx->ctx_ );
            return OpResult::Ok;
        }

        rx_end_.store( std::addressof( rx ), std::memory_order_release );
        Scheduler::self()->wait( & lk, true );
        // Tx should be ready
    }
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::send_for(
    ChanEnd & tx,
    ItemType & item,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return send_until( tx, item, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::send_until(
    ChanEnd & tx,
    ItemType & item,
    std::chrono::time_point< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    tx_end_.store( std::addressof( tx ), std::memory_order_release );
    Payload payload{ item };
    payload_.store( std::addressof( payload ), std::memory_order_release );

    auto rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        if ( rx->alt_ != nullptr ) {
            // FIXME
            /* rx->alt_->maybe_wakeup(); */
        } else {
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }

    if ( Scheduler::self()->wait_until( time_point, & lk ) ) {
        tx_end_.store( nullptr, std::memory_order_release );
        payload_.store( nullptr, std::memory_order_release );
        return OpResult::Timeout;
    } else {
        // Rx has consumed item
        return OpResult::Ok;
    }
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::recv_for(
    ChanEnd & rx,
    ItemType & item,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return recv_until( rx, item, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::recv_until(
    ChanEnd & rx,
    ItemType & item,
    std::chrono::time_point< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        auto tx = tx_end_.load( std::memory_order_acquire );
        if ( tx != nullptr ) {
            if ( tx->alt_ && ! alt_sync_.load( std::memory_order_acquire ) ) {
                // order matters here
                rx_end_.store( std::addressof( rx ), std::memory_order_release );
                // FIXME
                /* tx->alt_->maybe_wakeup(); */

                if ( Scheduler::self()->wait_until( time_point, & lk, true ) ) {
                    rx_end_.store( nullptr, std::memory_order_release );
                    return OpResult::Timeout;
                }

                if ( ! alt_sync_.load( std::memory_order_acquire ) ) {
                    continue;
                }
                // alting Tx synced, can now consume item
            }

            tx_end_.store( nullptr, std::memory_order_release );
            auto payload = payload_.exchange( nullptr, std::memory_order_acq_rel );
            BOOST_ASSERT( payload != nullptr );
            // order matters here
            item = std::move( payload->item_ );
            Scheduler::self()->schedule( tx->ctx_ );
            return OpResult::Ok;
        }

        rx_end_.store( std::addressof( rx ), std::memory_order_release );
        if ( Scheduler::self()->wait_until( time_point, & lk, true ) ) {
            rx_end_.store( nullptr, std::memory_order_release );
            return OpResult::Timeout;
        }
        // Tx should be ready
    }
}

template<typename T>
void ChannelImpl< T >::alt_send_enter( ChanEnd & tx ) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( std::addressof( tx ), std::memory_order_release );
    alt_sync_.store( false, std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_send_leave() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( nullptr, std::memory_order_release );
    alt_sync_.store( false, std::memory_order_release );
}

template<typename T>
bool ChannelImpl< T >::alt_send_ready( Alt * alt ) const noexcept
{
    BOOST_ASSERT( alt != nullptr );

    if ( is_closed() ) {
        return false;
    }
    auto rx = rx_end_.load( std::memory_order_acquire );
    return ( rx != nullptr ) && ( rx->alt_ != alt );
}

template<typename T>
AltResult ChannelImpl< T >::alt_send( ItemType & item ) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) != nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    auto rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx == nullptr ) {
        return AltResult::NoEnd;
    }

    Payload payload{ item };
    payload_.store( std::addressof( payload ), std::memory_order_release );

    if ( rx->alt_ != nullptr ) {
        if ( alt_sync_.load( std::memory_order_acquire ) ) {
            // accept the sync
            Scheduler::self()->schedule( rx->ctx_ );

            Scheduler::self()->wait( & lk );
            // Rx consumed item
            return AltResult::Ok;

        } else {
            // offer sync
            alt_sync_.store( true, std::memory_order_release );
            // FIXME
            // rx->alt_->maybe_wakeup();
            Scheduler::self()->wait( & lk );
            // if alt_sync is true, Rx consumed item
            return ( alt_sync_.load( std::memory_order_acquire ) )
                ? AltResult::Ok
                : AltResult::SyncFailed
                ;
        }

    } else {
        alt_sync_.store( true, std::memory_order_release );
        Scheduler::self()->schedule( rx->ctx_ );

        Scheduler::self()->wait( & lk );
        // Rx consumed item
        return AltResult::Ok;
    }
}


template<typename T>
void ChannelImpl< T >::alt_recv_enter( ChanEnd & rx ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( std::addressof( rx ), std::memory_order_release );
    // FIXME: make sure this does not leave a syncing end hanging
    alt_sync_.store( false, std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_recv_leave() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( nullptr, std::memory_order_release );
    // FIXME: make sure this does not leave a syncing end hanging
    alt_sync_.store( false, std::memory_order_release );
}

template<typename T>
bool ChannelImpl< T >::alt_recv_ready( Alt * alt ) const noexcept
{
    BOOST_ASSERT( alt != nullptr );

    if ( is_closed() ) {
        return false;
    }
    auto tx = tx_end_.load( std::memory_order_acquire );
    return ( tx != nullptr ) && ( tx->alt_ != alt );
}

template<typename T>
AltResult ChannelImpl< T >::alt_recv( ItemType & item ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) != nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    auto tx = tx_end_.load( std::memory_order_acquire );
    if ( tx == nullptr ) {
        return AltResult::NoEnd;
    }

    // showing empty branches for easier explanation
    if ( tx->alt_ != nullptr ) {
        if ( alt_sync_.load( std::memory_order_acquire ) ) {
            // accept the sync
        } else {
            // offer sync
            alt_sync_.store( true, std::memory_order_release );
            // FIXME
            // tx->alt_->maybe_wakeup();
            Scheduler::self()->wait( & lk );

            if ( ! alt_sync_.load( std::memory_order_acquire ) ) {
                return AltResult::SyncFailed;
            }
            // sync accepted
        }
    } else {
        // doesn't need to set alt_sync, as waking up tx
        // is a side-effect of syncing.
    }

    // complete the channel transaction
    tx_end_.store( nullptr, std::memory_order_release );
    auto payload = payload_.exchange( nullptr, std::memory_order_acq_rel );
    BOOST_ASSERT( payload != nullptr );
    // Order matters here
    item = std::move( payload->item_ );
    Scheduler::self()->schedule( tx->ctx_ );
    return AltResult::Ok;
}

} // namespace detail
} // namespace channel
PROXC_NAMESPACE_END

