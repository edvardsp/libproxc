
#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/channel/op_result.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

namespace channel {
namespace detail {

template<typename ItemT>
struct alignas(cache_alignment) ChanEnd
{
    Context *            ctx_;
    ItemT &              item_;
    alt::ChoiceBase *    alt_;
    ChanEnd( Context * ctx,
             ItemT & item,
             alt::ChoiceBase * alt = nullptr )
        : ctx_{ ctx }
        , item_{ item }
        , alt_{ alt }
    {}
};

template<typename ItemT>
struct AltEnd;

////////////////////////////////////////////////////////////////////////////////
// ChannelImpl
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class ChannelImpl
{
public:
    using ItemT = T;
    using EndT  = ChanEnd< ItemT >;
    using AltT  = AltEnd< ItemT >;

private:
    Spinlock    splk_{};

    alignas(cache_alignment) std::atomic< bool >    closed_{ false };

    alignas(cache_alignment) std::atomic< EndT * >    tx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >      tx_consumed_{ false };

    alignas(cache_alignment) std::atomic< EndT * >    rx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >      rx_consumed_{ false };

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
    OpResult send( EndT &, ItemT & ) noexcept;

    OpResult recv( EndT &, ItemT & ) noexcept;

    // send and receive methods with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( EndT &,
                       ItemT &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;

    template<typename Rep, typename Period>
    OpResult recv_for( EndT &,
                       ItemT &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;

    // send and receive methods with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( EndT &,
                         ItemT &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;

    template<typename Clock, typename Dur>
    OpResult recv_until( EndT &,
                         ItemT &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;

    // send and receive methods for alting
    void alt_send_enter( EndT & ) noexcept;
    void alt_send_leave() noexcept;
    bool alt_send_ready( Alt * ) const noexcept;
    AltResult alt_send( ItemT & ) noexcept;

    void alt_recv_enter( EndT & ) noexcept;
    void alt_recv_leave() noexcept;
    bool alt_recv_ready( Alt * ) const noexcept;
    AltResult alt_recv( ItemT & ) noexcept;
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
    EndT * tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( tx != nullptr ) {
        alt::ChoiceBase * alt_choice = tx->alt_choice_;
        if ( alt_choice != nullptr ) {
            alt_choice->maybe_wakeup();
        } else {
            Scheduler::self()->schedule( tx->ctx_ );
        }
    }
    EndT * rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        alt::ChoiceBase * alt_choice = tx->alt_choice_;
        if ( alt_choice != nullptr ) {
            alt_choice->maybe_wakeup();
        } else {
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
OpResult ChannelImpl< T >::send( EndT & tx, ItemT & item ) noexcept
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

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx != nullptr ) {
        alt::ChoiceBase * alt_choice = rx->alt_choice_;
        if ( alt_choice != nullptr ) {
            // doesn't care whether its selected or not
            (void)alt_choice->try_select();
        } else {
            rx_end_.store( nullptr, std::memory_order_release );
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }

    Scheduler::self()->wait( & lk );
    // Rx has consumed item
    // tx_end_ and payload_ should be zeroed by Rx
    BOOST_ASSERT( tx_end_.load( std::memory_order_acquire ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_acquire ) == nullptr );
    return OpResult::Ok;
}

template<typename T>
OpResult ChannelImpl< T >::recv( EndT & rx, ItemT & item ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        EndT * tx = tx_end_.load( std::memory_order_acquire );
        if ( tx != nullptr ) {
            bool tx_ready = false;
            auto alt_choice = tx->alt_choice_;
            if ( alt_choice != nullptr ) {
                // Tx is alting, must try to win selection
                if ( alt_choice->try_select() ) {
                    // "won" the selection, wakeup alt and
                    // wait for tx to set payload.
                    alt_choice->maybe_wakeup();
                    Scheduler::self()->wait( & lk, true );
                    // payload is now set.
                } else {
                    // "lost" the selection, wait until the same alting Tx
                    // or another Tx wakes up Rx.
                }

            } else {
                // Normal Tx, continue as normal
                tx_ready = true;
            }


            if ( tx_ready ) {
                tx_end_.store( nullptr, std::memory_order_release );
                auto payload = payload_.exchange( nullptr, std::memory_order_release );
                BOOST_ASSERT( payload != nullptr );
                // item must be consumed before scheduling tx
                item = std::move( payload->item_ );
                Scheduler::self()->schedule( tx->ctx_ );
                return OpResult::Ok;
            }
        }

        // wait because no Tx, or Alt Tx has not synced
        rx_end_.store( std::addressof( rx ), std::memory_order_release );
        Scheduler::self()->wait( & lk, true );
        // Tx should be ready
    }
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::send_for(
    EndT & tx,
    ItemT & item,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return send_until( tx, item, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::send_until(
    EndT & tx,
    ItemT & item,
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

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx != nullptr ) {
        alt::ChoiceBase * alt_choice = rx->alt_choice_;
        if ( alt_choice != nullptr ) {
            // doesn't care whether its selected or not, as Rx
            // wakes up Tx when payload is consumed. Tx does not care
            // whether its this alting Rx or another Rx which consumes it.
            (void)alt_choice->try_select();
        } else {
            rx_end_.store( nullptr, std::memory_order_release );
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }

    if ( ! Scheduler::self()->wait_until( time_point, & lk, true ) ) {
        tx_end_.store( nullptr, std::memory_order_release );
        payload_.store( nullptr, std::memory_order_release );
        return OpResult::Timeout;
    }
    // Rx has consumed item
    // tx_end_ and payload_ should be zeroed by Rx
    BOOST_ASSERT( tx_end_.load( std::memory_order_acquire ) == nullptr );
    BOOST_ASSERT( payload_.load( std::memory_order_acquire ) == nullptr );
    return OpResult::Ok;
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::recv_for(
    EndT & rx,
    ItemT & item,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return recv_until( rx, item, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::recv_until(
    EndT & rx,
    ItemT & item,
    std::chrono::time_point< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        EndT * tx = tx_end_.load( std::memory_order_acquire );
        if ( tx != nullptr ) {
            bool tx_ready = false;
            auto alt_choice = tx->alt_choice_;
            if ( alt_choice != nullptr ) {
                // Tx is alting, must try to win selection
                if ( alt_choice->try_select() ) {
                    // "won" the selection, wakeup alt and
                    // wait for tx to set payload. do not timeout here,
                    // as the actual transaction was agreed on before
                    // the timeout occured.
                    alt_choice->maybe_wakeup();
                    Scheduler::self()->wait( & lk, true );
                    // payload is now set.
                } else {
                    // "lost" the selection, wait until the same alting Tx
                    // or another Tx wakes up Rx.
                }

            } else {
                // Normal Tx, continue as normal
                tx_ready = true;
            }


            if ( tx_ready ) {
                tx_end_.store( nullptr, std::memory_order_release );
                auto payload = payload_.exchange( nullptr, std::memory_order_release );
                BOOST_ASSERT( payload != nullptr );
                // item must be consumed before scheduling tx
                item = std::move( payload->item_ );
                Scheduler::self()->schedule( tx->ctx_ );
                return OpResult::Ok;
            }
        }

        // wait because no Tx, or Alt Tx has not synced
        rx_end_.store( std::addressof( rx ), std::memory_order_release );
        if ( ! Scheduler::self()->wait_until( time_point, & lk, true ) ) {
            rx_end_.store( nullptr, std::memory_order_release );
            return OpResult::Timeout;
        }
        // Tx should be ready
    }
}

template<typename T>
void ChannelImpl< T >::alt_send_enter( EndT & tx ) noexcept
{
    // FIXME: commented out for now, as this does not allow multiple sends
    // from the same alt. Might delete this assert.
    /* BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) == nullptr ); */
    BOOST_ASSERT( payload_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( std::addressof( tx ), std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_send_leave() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( nullptr, std::memory_order_release );
    payload_.store( nullptr, std::memory_order_acquire );
}

template<typename T>
bool ChannelImpl< T >::alt_send_ready( Alt * alt ) const noexcept
{
    BOOST_ASSERT( alt != nullptr );

    if ( is_closed() ) {
        return false;
    }
    EndT * rx = rx_end_.load( std::memory_order_acquire );
    alt::ChoiceBase * alt_choice = rx->alt_choice_;
    return ( rx != nullptr ) &&
           ( ! alt_choice->same_alt( alt ) );
}

template<typename T>
AltResult ChannelImpl< T >::alt_send( ItemT & item ) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) != nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx == nullptr ) {
        return AltResult::NoEnd;
    }

    alt::ChoiceBase * alt_choice = rx->alt_choice_;
    if ( alt_choice != nullptr ) {

    } else {

    }
}


template<typename T>
void ChannelImpl< T >::alt_recv_enter( EndT & rx ) noexcept
{
    // FIXME: commented out for now, as this does not allow multiple recvs
    // from the same alt. Might delete this alter.
    /* BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr ); */

    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( std::addressof( rx ), std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_recv_leave() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( nullptr, std::memory_order_release );
}

template<typename T>
bool ChannelImpl< T >::alt_recv_ready( Alt * alt ) const noexcept
{
    BOOST_ASSERT( alt != nullptr );

    if ( is_closed() ) {
        return false;
    }
    auto tx = tx_end_.load( std::memory_order_acquire );
    return ( tx != nullptr ) &&
           ( tx->alt_ != alt );
}

template<typename T>
AltResult ChannelImpl< T >::alt_recv( ItemT & item ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) != nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return AltResult::Closed;
    }

}

} // namespace detail
} // namespace channel
PROXC_NAMESPACE_END

