
#pragma once

#include <atomic>
#include <chrono>
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

private:
    struct alignas(cache_alignment) Rendezvous {
        ItemType     item;
        Context *    ctx;

        Rendezvous(ItemType const & item_, Context * ctx_)
            : item{ item_ }
            , ctx{ ctx_ }
        {}

        Rendezvous(ItemType && item_, Context * ctx_)
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
    ~SyncChannel();

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
    OpResult send_(Rendezvous const &) noexcept;
    OpResult recv_(ItemType &) noexcept;
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
    return receiver_.load( std::memory_order_acquire ) == nullptr;
}

template<typename T>
void SyncChannel< T >::push_sender_(Rendezvous * rendezvous) noexcept
{
    BOOST_ASSERT( rendezvous != nullptr );
    BOOST_ASSERT( ! has_sender_() );
    return sender_.store(
        rendezvous,
        std::memory_order_release
    );
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
void SyncChannel< T >::push_receiver_(Context * ctx) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    BOOST_ASSERT( ! has_receiver_() );
    return receiver_.store(
        ctx,
        std::memory_order_release
    );
}

template<typename T>
Context * SyncChannel< T >::try_pop_receiver_() noexcept
{
    return sender_.exchange(
        nullptr,                    // desired
        std::memory_order_acq_rel   // order
    );
}

////////////////////////////////////////////////////////////////////////////////
// SyncChannel public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
SyncChannel< T >::~SyncChannel< T >()
{
    close();
    auto sender = try_pop_sender_();
    if (sender != nullptr) {
        BOOST_ASSERT( sender->ctx != nullptr );
        Scheduler::self()->schedule( sender->ctx );
    }
    auto receiver = try_pop_receiver_();
    if (receiver != nullptr) {
        Scheduler::self()->schedule( receiver );
    }

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
}

template<typename T>
OpResult SyncChannel< T >::send(ItemType const & item) noexcept
{
    auto running_ctx = Scheduler::running();
    Rendezvous rendezvous{ item, running_ctx };
    return send_( rendezvous );
}

template<typename T>
OpResult SyncChannel< T >::send(ItemType && item) noexcept
{
    auto running_ctx = Scheduler::running();
    Rendezvous rendezvous{ std::forward< ItemType >(item), running_ctx };
    return send_( rendezvous );
}

template<typename T>
OpResult SyncChannel< T >::send_(Rendezvous const & rendezvous) noexcept
{
    BOOST_ASSERT( ! has_sender_() );

    std::unique_lock< Spinlock > lk{ splk_ };
    // FIXME: implement synchronous unlocking of spinlocks
    // across Contexts when suspending current running context
    if ( is_closed() ) {
        return OpResult::Closed;
    }

    // Setup rendezvous
    push_sender_( & rendezvous );

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
OpResult SyncChannel< T >::recv(ItemType & item) noexcept
{
    return recv_( item );
}

template<typename T>
typename SyncChannel< T >::ItemType
SyncChannel< T >::recv()
{
    ItemType item;
    auto op_result = recv_( item );
    if (op_result != OpResult::Ok) {
        throw std::system_error();
    }
    return std::move( item );
}

template<typename T>
OpResult SyncChannel< T >::recv_(ItemType & item) noexcept
{
    BOOST_ASSERT( ! has_receiver_() );

    std::unique_lock< Spinlock > lk{ splk_ };
    // FIXME: implement synchronous unlocking of spinlocks
    // across Contexts when suspending current running context
    if ( is_closed() ) {
        return OpResult::Closed;
    }

    auto self = Scheduler::self();
    auto sender = try_pop_receiver_();

    if ( sender == nullptr ) {
        push_receiver_( self->running() );

        lk.unlock(); // FIXME: sync unlocking
        self->resume();
        lk.lock(); // FIXME: sync locking

        if ( is_closed() ) {
            return OpResult::Closed;
        }

        sender = try_pop_receiver_();
        BOOST_ASSERT( sender != nullptr );
    }

    item = std::move( sender->item );
    self->schedule( sender->ctx );

    BOOST_ASSERT( ! has_receiver_() );
    return OpResult::Ok;
}

} // namespace detail

template<typename T> class Tx;
template<typename T> class Rx;

template<typename T>
std::tuple< std::unique_ptr< Tx< T > >,
            std::unique_ptr< Rx< T > > >
create() noexcept
{
    auto channel = std::make_shared< detail::SyncChannel< T > >();
    return std::make_tuple( Tx< T >{ channel },
                            Rx< T >{ channel } );
}

template<typename T>
class Tx
{
private:
    using ItemType = T;
    using ChannelPtr = std::shared_ptr< detail::SyncChannel< T > >;

    ChannelPtr    chan_{ nullptr };

public:
    Tx();
    ~Tx();

    // make non-copyable
    Tx(Tx const &)               = delete;
    Tx & operator = (Tx const &) = delete;

    OpResult send(ItemType const &) noexcept;

private:
    Tx(ChannelPtr ptr);

    friend
    std::tuple< std::unique_ptr< Tx< T > >,
                std::unique_ptr< Rx< T > > >
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
    Rx();
    ~Rx();

    // make non-copyable
    Rx(Rx const &)               = delete;
    Rx & operator = (Rx const &) = delete;

    OpResult recv(ItemType &) noexcept;
    ItemType recv() noexcept;

private:
    Rx(ChannelPtr ptr);


    friend
    std::tuple< std::unique_ptr< Tx< T > >,
                std::unique_ptr< Rx< T > > >
    create() noexcept;
};

} // namespace sync
} // namespace channel
PROXC_NAMESPACE_END

