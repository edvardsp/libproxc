
#pragma once

#include <atomic>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <tuple>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/channel/op_result.hpp>

#include <boost/assert.hpp>
#include <boost/intrusive_ptr.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {
namespace sync {
namespace detail {

template<typename T>
class SyncChannel
{
public:
    using ItemType = T;

    enum class Result {
        Ok,
        Timeout,
        Closed,
        Error,
    };

private:
    struct alignas(cache_alignment) Rendezvous {
        ItemType     item;
        Context *    ctx;

        Rendezvous( ItemType const & item_, Context * ctx_ )
            : item{ item_ }
            , ctx{ ctx_ }
        {}

        Rendezvous( ItemType && item_, Context * ctx_ )
            : item{ std::forward< ItemType >( item_ ) }
            , ctx{ ctx_ }
        {}
    };

    alignas(cache_alignment) std::atomic< Rendezvous * >    sender_{ nullptr };
    alignas(cache_alignment) std::atomic< Context *>        receiver_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >            closed_{ false };

    Spinlock    splk_{};

    bool has_sender_() const noexcept;
    bool has_receiver_() const noexcept;
    void push_sender_(Rendezvous *) noexcept;
    void push_receiver_(Context *) noexcept;
    Rendezvous * try_pop_sender_() noexcept;
    Context * try_pop_receiver_() noexcept;

public:
    SyncChannel() = default;
    ~SyncChannel() noexcept;

    // make non-copyable
    SyncChannel(SyncChannel const &)               = delete;
    SyncChannel & operator = (SyncChannel const &) = delete;

    bool is_closed() const noexcept;
    void close() noexcept;

    OpResult send(ItemType const &) noexcept;
    OpResult send(ItemType &&) noexcept;

    OpResult recv(ItemType &) noexcept;
    ItemType recv();

private:
    OpResult send_impl_(Rendezvous &) noexcept;
    OpResult recv_impl_(ItemType &) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// SyncChannel private methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
bool SyncChannel< T >::has_sender_() const noexcept
{
    return sender_.load( std::memory_order_acquire ) != nullptr;
}

template<typename T>
bool SyncChannel< T >::has_receiver_() const noexcept
{
    return receiver_.load( std::memory_order_acquire ) != nullptr;
}

template<typename T>
void SyncChannel< T >::push_sender_( Rendezvous * rendezvous ) noexcept
{
    BOOST_ASSERT( rendezvous != nullptr );
    BOOST_ASSERT( ! has_sender_() );
    sender_.store( rendezvous, std::memory_order_release );
}

template<typename T>
typename SyncChannel< T >::Rendezvous *
SyncChannel< T >::try_pop_sender_() noexcept
{
    return sender_.exchange(
        nullptr,                    // desired
        std::memory_order_acq_rel   // order
    );
}

template<typename T>
void SyncChannel< T >::push_receiver_( Context * ctx ) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    BOOST_ASSERT( ! has_receiver_() );
    receiver_.store( ctx, std::memory_order_release );
}

template<typename T>
Context * SyncChannel< T >::try_pop_receiver_() noexcept
{
    return receiver_.exchange(
        nullptr,                    // desired
        std::memory_order_acq_rel   // order
    );
}

////////////////////////////////////////////////////////////////////////////////
// SyncChannel public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
SyncChannel< T >::~SyncChannel< T >() noexcept
{
    close();

    BOOST_ASSERT( ! has_sender_() );
    BOOST_ASSERT( ! has_receiver_() );
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
    auto sender = try_pop_sender_();
    if ( sender != nullptr ) {
        BOOST_ASSERT( sender->ctx != nullptr );
        Scheduler::self()->schedule( sender->ctx );
    }
    auto receiver = try_pop_receiver_();
    if ( receiver != nullptr ) {
        Scheduler::self()->schedule( receiver );
    }
}

template<typename T>
OpResult SyncChannel< T >::send( ItemType const & item ) noexcept
{
    auto running_ctx = Scheduler::running();
    Rendezvous rendezvous{ item, running_ctx };
    return send_impl_( rendezvous );
}

template<typename T>
OpResult SyncChannel< T >::send( ItemType && item ) noexcept
{
    auto running_ctx = Scheduler::running();
    Rendezvous rendezvous{ std::forward< ItemType >( item ), running_ctx };
    return send_impl_( rendezvous );
}

template<typename T>
OpResult SyncChannel< T >::send_impl_( Rendezvous & rendezvous ) noexcept
{
    BOOST_ASSERT( ! has_sender_() );

    // FIXME: implement synchronous unlocking of spinlocks
    // across Contexts when suspending current running context
    std::unique_lock< Spinlock > lk{ splk_ };
    if ( is_closed() ) {
        return OpResult::Closed;
    }

    // Setup rendezvous
    push_sender_( std::addressof( rendezvous ) );

    auto self = Scheduler::self();
    auto receiver = try_pop_receiver_();

    // if receiver is ready, complete rendezvous
    if ( receiver != nullptr ) {
        self->schedule( receiver );
    }

    lk.unlock(); // FIXME: sync unlocking
    // Receiver consumes item
    self->resume();
    // Resumed
    lk.lock();

    BOOST_ASSERT( ! has_sender_() );

    // Check once more if channel is closed
    if ( is_closed() ) {
        return OpResult::Closed;
    } else {
        // Value has been consumed
        return OpResult::Ok;
    }
}

template<typename T>
OpResult SyncChannel< T >::recv( ItemType & item ) noexcept
{
    return recv_impl_( item );
}

template<typename T>
typename SyncChannel< T >::ItemType
SyncChannel< T >::recv()
{
    ItemType item;
    auto op_result = recv_impl_( item );
    if ( op_result != OpResult::Ok ) {
        throw std::system_error();
    }
    return std::move( item );
}

template<typename T>
OpResult SyncChannel< T >::recv_impl_( ItemType & item ) noexcept
{
    BOOST_ASSERT( ! has_receiver_() );

    auto running_ctx = Scheduler::running();
    std::unique_lock< Spinlock > lk{ splk_ };
    Rendezvous * r = nullptr;
    for (;;) {
        if ( is_closed() ) {
            return OpResult::Closed;
        }

        r = try_pop_sender_();
        if ( r != nullptr ) {
            lk.unlock();
            item = std::move( r->item );
            Scheduler::self()->schedule( r->ctx );
            return OpResult::Ok;

        } else {
            push_receiver_( running_ctx );
            lk.unlock(); // FIXME: sync unlocking
            Scheduler::self()->resume();
            lk.lock();
        }

    }
}

} // namespace detail

template<typename T> class Tx;
template<typename T> class Rx;

template<typename T>
class Tx
{
private:
    using ItemType = T;
    using ChannelPtr = std::shared_ptr< detail::SyncChannel< ItemType > >;

    ChannelPtr    chan_{ nullptr };

public:
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

    OpResult send( ItemType const & item ) noexcept
    { return chan_->send( item ); }

    OpResult send( ItemType && item ) noexcept
    { return chan_->send( std::forward< ItemType>( item ) ); }

private:
    Tx( ChannelPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;
};

template<typename T>
class Rx
{
private:
    using ItemType = T;
    using ChannelPtr = std::shared_ptr< detail::SyncChannel< T > >;

    ChannelPtr    chan_{ nullptr };

public:
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

    OpResult recv( ItemType & item ) noexcept
    { return chan_->recv( item ); }

    ItemType recv() noexcept
    { return std::move( chan_->recv() ); }

private:
    Rx( ChannelPtr ptr )
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

} // namespace sync
} // namespace channel
PROXC_NAMESPACE_END

