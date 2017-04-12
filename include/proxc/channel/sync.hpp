
#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/channel/base.hpp>
#include <proxc/channel/op_result.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {
namespace sync {
namespace detail {

// forward declaration
class Alt;

////////////////////////////////////////////////////////////////////////////////
// SyncChannel
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class SyncChannel : ::proxc::channel::detail::ChannelBase
{
public:
    using ItemType = T;

    template<typename Rep, typename Period>
    using Duration = std::chrono::duration< Rep, Period >;
    template<typename Clock, typename Dur>
    using TimePoint = std::chrono::time_point< Clock, Dur >;

private:
    struct alignas(cache_alignment) ChanEnd
    {
        Context *     ctx_;
        ItemType &    item_;
        Alt *         alt_;
        ChanEnd( Context * ctx, ItemType & item, Alt * alt = nullptr )
            : ctx_{ ctx }, item_{ item }, alt_{ alt }
        {}
    };

    Spinlock    splk_{};

    alignas(cache_alignment) std::atomic< ChanEnd * >    tx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< ChanEnd * >    rx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >         closed_{ false };

    // used to synchronize during alting, on either Tx or Rx end
    alignas(cache_alignment) std::atomic< bool >    alt_sync_{ false };

    void set_tx_( ChanEnd * ) noexcept;
    void set_rx_( ChanEnd * ) noexcept;
    ChanEnd * get_tx_() const noexcept;
    ChanEnd * get_rx_() const noexcept;
    ChanEnd * pop_tx_() noexcept;
    ChanEnd * pop_rx_() noexcept;

    bool is_sync() const noexcept;
    void set_sync( bool ) noexcept;

public:
    SyncChannel() = default;
    ~SyncChannel() noexcept;

    // make non-copyable
    SyncChannel( SyncChannel const & )               = delete;
    SyncChannel & operator = ( SyncChannel const & ) = delete;

    bool is_closed() const noexcept;
    void close() noexcept;

    bool has_tx_() const noexcept;
    bool has_rx_() const noexcept;

    // normal send and receieve methods
    OpResult send( ItemType const & ) noexcept;
    OpResult send( ItemType && ) noexcept;

    OpResult recv( ItemType & ) noexcept;
    ItemType recv();

    // send and receive methods with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( ItemType const &, Duration< Rep, Period > const & ) noexcept;
    template<typename Rep, typename Period>
    OpResult send_for( ItemType &&, Duration< Rep, Period > const & ) noexcept;
    template<typename Rep, typename Period>
    OpResult recv_for( ItemType &, Duration< Rep, Period > const & ) noexcept;

    // send and receive methods with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( ItemType const &, TimePoint< Clock, Dur > const & ) noexcept;
    template<typename Clock, typename Dur>
    OpResult send_until( ItemType &&, TimePoint< Clock, Dur > const & ) noexcept;
    template<typename Clock, typename Dur>
    OpResult recv_until( ItemType &, TimePoint< Clock, Dur > const & ) noexcept;

    OpResult alt_send( ItemType const &, Alt * ) noexcept;
    OpResult alt_send( ItemType &&, Alt * ) noexcept;
    OpResult alt_recv( ItemType &, Alt * ) noexcept;

private:
    OpResult send_impl_( ChanEnd & ) noexcept;
    OpResult recv_impl_( ChanEnd & ) noexcept;

    template<typename Clock, typename Dur>
    OpResult send_timeout_impl_( ChanEnd &, TimePoint< Clock, Dur > const & ) noexcept;
    template<typename Clock, typename Dur>
    OpResult recv_timeout_impl_( ChanEnd &, TimePoint< Clock, Dur > const & ) noexcept;

    OpResult alt_send_impl_( ChanEnd & ) noexcept;
    OpResult alt_recv_impl_( ChanEnd & ) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// SyncChannel private methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
bool SyncChannel< T >::has_tx_() const noexcept
{
    return get_tx_() != nullptr;
}

template<typename T>
bool SyncChannel< T >::has_rx_() const noexcept
{
    return get_rx_() != nullptr;
}

template<typename T>
void SyncChannel< T >::set_tx_( ChanEnd * tx_end ) noexcept
{
    tx_end_.store( tx_end, std::memory_order_release );
}

template<typename T>
void SyncChannel< T >::set_rx_( ChanEnd * rx_end ) noexcept
{
    rx_end_.store( rx_end, std::memory_order_release );
}

template<typename T>
auto SyncChannel< T >::get_tx_() const noexcept
    -> ChanEnd *
{
    return tx_end_.load( std::memory_order_acquire );
}

template<typename T>
auto SyncChannel< T >::get_rx_() const noexcept
    -> ChanEnd *
{
    return rx_end_.load( std::memory_order_acquire );
}

template<typename T>
auto SyncChannel< T >::pop_tx_() noexcept
    -> ChanEnd *
{
    return tx_end_.exchange( nullptr, std::memory_order_acq_rel );
}

template<typename T>
auto SyncChannel< T >::pop_rx_() noexcept
    -> ChanEnd *
{
    return rx_end_.exchange( nullptr, std::memory_order_acq_rel );
}

template<typename T>
bool SyncChannel< T >::is_sync() const noexcept
{
    return alt_sync_.load( std::memory_order_acquire );
}

template<typename T>
void SyncChannel< T >::set_sync( bool value ) noexcept
{
    alt_sync_.store( value, std::memory_order_release );
}

////////////////////////////////////////////////////////////////////////////////
// SyncChannel public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
SyncChannel< T >::~SyncChannel< T >() noexcept
{
    close();

    BOOST_ASSERT( ! has_tx_() );
    BOOST_ASSERT( ! has_rx_() );
}

template<typename T>
bool SyncChannel< T >::is_closed() const noexcept
{
    return closed_.load( std::memory_order_acquire );
}

template<typename T>
void SyncChannel< T >::close() noexcept
{
    closed_.store( true, std::memory_order_release );
    auto tx = pop_tx_();
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
    auto rx = pop_rx_();
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
OpResult SyncChannel< T >::send( ItemType const & item ) noexcept
{
    ItemType i{ item };
    ChanEnd tx{ Scheduler::running(), i };
    return send_impl_( tx );
}

template<typename T>
OpResult SyncChannel< T >::send( ItemType && item ) noexcept
{
    ChanEnd tx{ Scheduler::running(), item };
    return send_impl_( tx );
}

template<typename T>
OpResult SyncChannel< T >::recv( ItemType & item ) noexcept
{
    ChanEnd rx{ Scheduler::running(), item };
    return recv_impl_( rx );
}

template<typename T>
typename SyncChannel< T >::ItemType
SyncChannel< T >::recv()
{
    ItemType item;
    ChanEnd rx{ Scheduler::running(), item };
    auto op_result = recv_impl_( rx );
    if ( op_result != OpResult::Ok ) {
        throw std::system_error();
    }
    return std::move( item );
}

template<typename T>
template<typename Rep, typename Period>
OpResult SyncChannel< T >::send_for(
    ItemType const & item,
    Duration< Rep, Period > const & duration
) noexcept
{
    ItemType i{ item };
    ChanEnd tx{ Scheduler::running(), i };
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return send_timeout_impl_( tx, timepoint );
}

template<typename T>
template<typename Rep, typename Period>
OpResult SyncChannel< T >::send_for(
    ItemType && item,
    Duration< Rep, Period > const & duration
) noexcept
{
    ChanEnd tx{ Scheduler::running(), item };
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return send_timeout_impl_( tx, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult SyncChannel< T >::send_until(
    ItemType const & item,
    TimePoint< Clock, Dur > const & timepoint
) noexcept
{
    ItemType i{ item };
    ChanEnd tx{ Scheduler::running(), i };
    return send_timeout_impl_( tx, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult SyncChannel< T >::send_until(
    ItemType && item,
    TimePoint< Clock, Dur > const & timepoint
) noexcept
{
    ChanEnd tx{ Scheduler::running(), item };
    return send_timeout_impl_( tx, timepoint );
}

template<typename T>
template<typename Rep, typename Period>
OpResult SyncChannel< T >::recv_for(
    ItemType & item,
    Duration< Rep, Period > const & duration
) noexcept
{
    ChanEnd rx{ Scheduler::running(), item };
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return recv_timeout_impl_( rx, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult SyncChannel< T >::recv_until(
    ItemType & item,
    TimePoint< Clock, Dur > const & timepoint
) noexcept
{
    ChanEnd rx{ Scheduler::running(), item };
    return recv_timeout_impl_( rx, timepoint );
}

template<typename T>
OpResult SyncChannel< T >::alt_send( ItemType const & item, Alt * alt ) noexcept
{
    BOOST_ASSERT( alt != nullptr );
    ItemType i{ item };
    ChanEnd alt_tx{ Scheduler::running(), i, alt };
    return alt_send_impl_( alt_tx );
}

template<typename T>
OpResult SyncChannel< T >::alt_send( ItemType && item, Alt * alt ) noexcept
{
    BOOST_ASSERT( alt != nullptr );
    ChanEnd alt_tx{ Scheduler::running(), item, alt };
    return alt_send_impl_( alt_tx );
}

template<typename T>
OpResult SyncChannel< T >::alt_recv( ItemType & item, Alt * alt ) noexcept
{
    BOOST_ASSERT( alt != nullptr );
    ChanEnd alt_rx{ Scheduler::running(), item, alt };
    return alt_recv_impl_( alt_rx );
}

////////////////////////////////////////////////////////////////////////////////
// SyncChannel private methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
OpResult SyncChannel< T >::send_impl_( ChanEnd & tx ) noexcept
{
    BOOST_ASSERT( ! has_tx_() );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    set_tx_( & tx );

    auto rx = pop_rx_();
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
OpResult SyncChannel< T >::recv_impl_( ChanEnd & rx ) noexcept
{
    BOOST_ASSERT( ! has_rx_() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        auto tx = get_tx_();
        if ( tx != nullptr ) {
            if ( tx->alt_ && ! is_sync() ) {
                // order matters here
                set_rx_( & rx );
                // FIXME
                /* tx->alt_->maybe_wakeup(); */

                Scheduler::self()->wait( & lk, true );

                if ( ! is_sync() ) {
                    continue;
                }
                // alting Tx synced, can now consume item
            }

            set_tx_( nullptr );
            // order matters here
            rx.item_ = std::move( tx->item_ );
            Scheduler::self()->schedule( tx->ctx_ );
            return OpResult::Ok;
        }

        set_rx_( & rx );
        Scheduler::self()->wait( & lk, true );
        // Tx should be ready
    }
}

template<typename T>
template<typename Clock, typename Dur>
OpResult SyncChannel< T >::send_timeout_impl_(
    ChanEnd & tx,
    TimePoint< Clock, Dur > const & time_point
) noexcept
{

    BOOST_ASSERT( ! has_tx_() );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    set_tx_( & tx );

    auto rx = pop_rx_();
    if ( rx != nullptr ) {
        if ( rx->alt_ != nullptr ) {
            // FIXME
            /* rx->alt_->maybe_wakeup(); */
        } else {
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }

    if ( Scheduler::self()->wait_until( time_point, & lk ) ) {
        set_tx_( nullptr );
        return OpResult::Timeout;
    } else {
        // Rx has consumed item
        return OpResult::Ok;
    }
}

template<typename T>
template<typename Clock, typename Dur>
OpResult SyncChannel< T >::recv_timeout_impl_(
     ChanEnd & rx,
     TimePoint< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( ! has_rx_() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        auto tx = get_tx_();
        if ( tx != nullptr ) {
            if ( tx->alt_ && ! is_sync() ) {
                // order matters here
                set_rx_( & rx );
                // FIXME
                /* tx->alt_->maybe_wakeup(); */

                if ( Scheduler::self()->wait_until( time_point, & lk, true ) ) {
                    set_rx_( nullptr );
                    return OpResult::Timeout;
                }

                if ( ! is_sync() ) {
                    continue;
                }
                // alting Tx synced, can now consume item
            }

            set_tx_( nullptr );
            // order matters here
            rx.item_ = std::move( tx->item_ );
            Scheduler::self()->schedule( tx->ctx_ );
            return OpResult::Ok;
        }

        set_rx_( & rx );
        if ( Scheduler::self()->wait_until( time_point, & lk, true ) ) {
            set_rx_( nullptr );
            return OpResult::Timeout;
        }
        // Tx should be ready
    }
}

template<typename T>
OpResult SyncChannel< T >::alt_send_impl_( ChanEnd & alt_tx ) noexcept
{


}

template<typename T>
OpResult SyncChannel< T >::alt_recv_impl_( ChanEnd & alt_rx ) noexcept
{

}

} // namespace detail

template<typename T> class Tx;
template<typename T> class Rx;

////////////////////////////////////////////////////////////////////////////////
// SyncChannel Tx
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class Tx : ::proxc::channel::detail::TxBase
{
public:
    using ItemType = T;

private:
    using ChanType = detail::SyncChannel< ItemType >;
    using ChanPtr = std::shared_ptr< ChanType >;

    ChanPtr    chan_{ nullptr };

public:
    template<typename Rep, typename Period>
    using Duration = typename ChanType::template Duration< Rep, Period >;
    template<typename Clock, typename Dur>
    using TimePoint = typename ChanType::template TimePoint< Clock, Dur >;

    Tx() = default;
    ~Tx()
    { if ( chan_ ) { chan_->close(); } }

    // make non-copyable
    Tx( Tx const & )               = delete;
    Tx & operator = ( Tx const & ) = delete;

    // make movable
    Tx( Tx && ) = default;
    Tx & operator = ( Tx && ) = default;

    bool is_closed() const noexcept
    { return chan_->is_closed(); }

    void close() noexcept
    {
        chan_->close();
        chan_.reset();
    }

    bool ready() const noexcept
    { return chan_->has_rx_(); }

    // normal send operations
    OpResult send( ItemType const & item ) noexcept
    { return chan_->send( item ); }

    OpResult send( ItemType && item ) noexcept
    { return chan_->send( std::move( item ) ); }

    // send operations with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( ItemType const & item, Duration< Rep, Period > const & duration ) noexcept
    { return chan_->send_for( item, duration ); }

    template<typename Rep, typename Period>
    OpResult send_for( ItemType && item, Duration< Rep, Period > const & duration ) noexcept
    { return chan_->send_for( std::move( item ), duration ); }

    // send operations with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( ItemType const & item, TimePoint< Clock, Dur > const & timepoint ) noexcept
    { return chan_->send_until( item, timepoint ); }

    template<typename Clock, typename Dur>
    OpResult send_until( ItemType && item, TimePoint< Clock, Dur > const & timepoint ) noexcept
    { return chan_->send_until( std::move( item ), timepoint ); }

private:
    Tx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// SyncChannel Rx
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class Rx : ::proxc::channel::detail::RxBase
{
public:
    using ItemType = T;

private:
    using ChanType = detail::SyncChannel< ItemType >;
    using ChanPtr = std::shared_ptr< ChanType >;

    ChanPtr    chan_{ nullptr };

public:
    template<typename Rep, typename Period>
    using Duration = typename ChanType::template Duration< Rep, Period >;
    template<typename Clock, typename Dur>
    using TimePoint = typename ChanType::template TimePoint< Clock, Dur >;

    Rx() = default;
    ~Rx()
    { if ( chan_ ) { chan_->close(); } }

    // make non-copyable
    Rx( Rx const & )               = delete;
    Rx & operator = ( Rx const & ) = delete;

    // make movable
    Rx( Rx && )               = default;
    Rx & operator = ( Rx && ) = default;

    bool is_closed() const noexcept
    { return chan_->is_closed(); }

    void close() noexcept
    {
        chan_->close();
        chan_.reset();
    }

    bool ready() const noexcept
    { return chan_->has_tx_(); }

    // normal recv operations
    OpResult recv( ItemType & item ) noexcept
    { return chan_->recv( item ); }

    ItemType recv() noexcept
    { return std::move( chan_->recv() ); }

    // recv operation with duration timeout
    template<typename Rep, typename Period>
    OpResult recv_for( ItemType & item, Duration< Rep, Period > const & duration ) noexcept
    { return chan_->recv_for( item, duration ); }

    // recv operation with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult recv_until( ItemType & item, TimePoint< Clock, Dur > const & timepoint ) noexcept
    { return chan_->recv_until( item, timepoint ); }

private:
    Rx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

public:
    class Iterator
        : public std::iterator<
            std::input_iterator_tag,
            typename std::remove_reference< ItemType >::type
          >
    {
    private:
        using StorageType = typename std::aligned_storage<
            sizeof(ItemType), alignof(ItemType)
        >::type;

        Rx< T > *      rx_{ nullptr };
        StorageType    storage_;

        void increment() noexcept
        {
            BOOST_ASSERT( rx_ != nullptr );
            ItemType item;
            auto res = rx_->recv( item );
            if ( res == OpResult::Ok ) {
                auto addr = static_cast< void * >( std::addressof( storage_ ) );
                ::new (addr) ItemType{ std::move( item ) };
            } else {
                rx_ = nullptr;
            }
        }

    public:
        using Ptr = typename Iterator::pointer;
        using Ref = typename Iterator::reference;

        Iterator() noexcept = default;

        explicit Iterator( Rx< T > * rx ) noexcept
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
        { return * reinterpret_cast< ItemType * >( std::addressof( storage_ ) ); }

        Ptr operator -> () noexcept
        { return reinterpret_cast< ItemType * >( std::addressof( storage_ ) ); }
    };
};

template<typename T>
typename Rx< T >::Iterator
begin( Rx< T > & chan )
{
    return typename Rx< T >::Iterator( & chan );
}

template<typename T>
typename Rx< T >::Iterator
end( Rx< T > & )
{
    return typename Rx< T >::Iterator();
}

template<typename T>
std::tuple< Tx< T >, Rx< T > >
create() noexcept
{
    auto channel = std::make_shared< detail::SyncChannel< T > >();
    return std::make_tuple( Tx< T >{ channel }, Rx< T >{ channel } );
}

template<typename T>
std::tuple<
    std::vector< Tx< T > >,
    std::vector< Rx< T > >
>
create_n( const std::size_t n ) noexcept
{
    std::vector< Tx< T > > txs;
    std::vector< Rx< T > > rxs;
    txs.reserve( n );
    rxs.reserve( n );
    for( std::size_t i = 0; i < n; ++i ) {
        auto ch = create< T >();
        txs.push_back( std::move( std::get<0>( ch ) ) );
        rxs.push_back( std::move( std::get<1>( ch ) ) );
    }
    return std::make_tuple( std::move( txs ), std::move( rxs ) );
}

} // namespace sync
} // namespace channel
PROXC_NAMESPACE_END

