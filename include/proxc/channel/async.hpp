
#pragma once

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
#include <proxc/detail/spsc_queue.hpp>

#include <boost/assert.hpp>

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-private-field"
#endif

PROXC_NAMESPACE_BEGIN
namespace channel {
namespace async {
namespace detail {

template<typename T>
class AsyncChannel : public ::proxc::channel::detail::ChannelBase
{
public:
    using ItemType = T;

private:
    using BufferType = proxc::detail::SpscQueue< ItemType >;

    enum {
        NICE_LEVEL_CAP = 10, // number of tx which will cause a yield
    };

    alignas(cache_alignment) BufferType *    buffer_;

    alignas(cache_alignment) std::atomic< Context * >    receiver_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >         closed_{ false };

    std::size_t    nice_sending_{ 0 };

    Spinlock    splk_{};
    char        padding_[cacheline_length];

public:
    AsyncChannel();
    ~AsyncChannel() noexcept;

    // make non-copyable
    AsyncChannel( AsyncChannel const & )               = delete;
    AsyncChannel & operator = ( AsyncChannel const & ) = delete;

    bool is_closed() const noexcept;
    void close() noexcept;
    bool is_empty() const noexcept;

    OpResult send( ItemType const & ) noexcept;
    OpResult send( ItemType && ) noexcept;

    OpResult recv( ItemType & ) noexcept;
    ItemType recv();

private:
    bool has_receiver_() const noexcept;
    void push_receiver_( Context * ) noexcept;
    Context * try_pop_receiver_() noexcept;

    OpResult send_impl_( ItemType ) noexcept;
    OpResult recv_impl_( ItemType & ) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// AsyncChannel public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
AsyncChannel< T >::AsyncChannel()
    : buffer_{ new BufferType{} }
{
}

template<typename T>
AsyncChannel< T >::~AsyncChannel() noexcept
{
    close();

    delete buffer_;

    // only receiver will ever be blocked
    BOOST_ASSERT( ! has_receiver_() );
    BOOST_ASSERT( is_closed() );
}

template<typename T>
bool AsyncChannel< T >::is_closed() const noexcept
{
    return closed_.load( std::memory_order_acquire );
}

template<typename T>
void AsyncChannel< T >::close() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    closed_.store( true, std::memory_order_release );
    auto receiver = try_pop_receiver_();
    if ( receiver != nullptr ) {
        Scheduler::self()->schedule( receiver );
    }
}

template<typename T>
bool AsyncChannel< T >::is_empty() const noexcept
{
    return buffer_->is_empty();
}

template<typename T>
OpResult AsyncChannel< T >::send( ItemType const & item ) noexcept
{
    return send_impl_( item );
}

template<typename T>
OpResult AsyncChannel< T >::send( ItemType && item ) noexcept
{
    return send_impl_( std::move( item ) );
}

template<typename T>
OpResult AsyncChannel< T >::recv( ItemType & item ) noexcept
{
    return recv_impl_( item );
}

template<typename T>
typename AsyncChannel< T >::ItemType
AsyncChannel< T >::recv()
{
    ItemType item;
    auto res = recv_impl_( item );
    if ( res != OpResult::Ok ) {
        throw std::system_error();
    }
    return std::move( item );
}

////////////////////////////////////////////////////////////////////////////////
// AsyncChannel private methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
bool AsyncChannel< T >::has_receiver_() const noexcept
{
    return receiver_.load( std::memory_order_acquire ) != nullptr;
}

template<typename T>
void AsyncChannel< T >::push_receiver_( Context * ctx ) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    BOOST_ASSERT( ! has_receiver_() );
    receiver_.store( ctx, std::memory_order_release );
}

template<typename T>
Context * AsyncChannel< T >::try_pop_receiver_() noexcept
{
    return receiver_.exchange(
        nullptr,                   // desired
        std::memory_order_acq_rel  // order
    );
}

template<typename T>
OpResult AsyncChannel< T >::send_impl_( ItemType item ) noexcept
{
    if ( is_closed() ) {
        return OpResult::Closed;
    }

    buffer_->enqueue( std::move( item ) );

    {
        std::unique_lock< Spinlock > lk{ splk_ };
        auto receiver = try_pop_receiver_();
        if ( receiver != nullptr ) {
            Scheduler::self()->schedule( receiver );
        }
    }

    if ( nice_sending_++ >= NICE_LEVEL_CAP ) {
        nice_sending_ = 0;
        Scheduler::self()->yield();
    }

    return OpResult::Ok;
}

template<typename T>
OpResult AsyncChannel< T >::recv_impl_( ItemType & item ) noexcept
{
    BOOST_ASSERT( ! has_receiver_() );
    auto running_ctx = Scheduler::running();
    while ( true ) {
        if ( buffer_->dequeue( item ) ) {
            return OpResult::Ok;

        } else {
            std::unique_lock< Spinlock > lk{ splk_ };
            if ( ! is_empty() ) {
                continue;
            } else if ( is_closed() ) {
                return OpResult::Closed;
            }

            push_receiver_( running_ctx );
            lk.unlock(); // FIXME: sync unlocking
            Scheduler::self()->resume();
        }
    }
}

} // namespace detail

template<typename T> class Tx;
template<typename T> class Rx;

template<typename T>
class Tx : public ::proxc::channel::detail::TxBase
{
private:
    using ItemType = T;
    using ChannelPtr = std::shared_ptr< detail::AsyncChannel< ItemType > >;

    ChannelPtr    chan_{ nullptr };

public:
    Tx() = default;
    ~Tx()
    { if ( chan_ ) { chan_->close(); } }

    // make non-copyable
    Tx( Tx const & )               = delete;
    Tx & operator = ( Tx const & ) = delete;

    // make moveable
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
    { return chan_->send( std::move( item ) ); }

private:
    Tx( ChannelPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;
};

template<typename T>
class Rx : public ::proxc::channel::detail::RxBase
{
private:
    using ItemType = T;
    using ChannelPtr = std::shared_ptr< detail::AsyncChannel< ItemType > >;

    ChannelPtr    chan_{ nullptr };

public:
    Rx() = default;
    ~Rx()
    { if ( chan_ ) { chan_->close(); } }

    // make non-copyable
    Rx( Rx const & )               = delete;
    Rx & operator = ( Rx const & ) = delete;

    // make moveable
    Rx( Rx && ) = default;
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

    OpResult recv()
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
    auto channel = std::make_shared< detail::AsyncChannel< T > >();
    return std::make_tuple( Tx< T >{ channel }, Rx< T >{ channel } );
}

template<typename T>
std::vector< decltype(create< T >()) >
create_n( const std::size_t n ) noexcept
{
    std::vector< decltype(create< T >()) > chans;
    chans.reserve( n );
    for( std::size_t i = 0; i < n; ++i ) {
        chans.push_back( create< T >() );
    }
    return std::move( chans );
}

} // namespace async
} // namespace channel
PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif
