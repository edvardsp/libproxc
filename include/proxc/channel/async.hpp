
#pragma once

#include <atomic>
#include <chrono>
#include <iterator>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/channel/op_result.hpp>
#include <proxc/detail/circular_array.hpp>

#include <boost/assert.hpp>

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunused-private-field"
#endif

PROXC_NAMESPACE_BEGIN
namespace channel {
namespace async {
namespace detail {

constexpr std::ptrdiff_t to_signed( std::size_t x ) noexcept
{
    constexpr std::size_t ptrdiff_max = static_cast< std::size_t >( PTRDIFF_MAX );
    constexpr std::size_t ptrdiff_min = static_cast< std::size_t >( PTRDIFF_MIN );
    static_assert( ptrdiff_max + 1 == ptrdiff_min, "Wrong unsigned integer wrapping behaviour");

    if ( x > ptrdiff_max ) {
        return static_cast< std::ptrdiff_t >( x - ptrdiff_min ) + PTRDIFF_MIN;
    } else {
        return static_cast< std::ptrdiff_t >( x );
    }
}

template<typename T>
class AsyncChannel
{
public:
    using ItemType = T;

    static constexpr std::size_t default_capacity = 64;

private:
    using ArrayType = proxc::detail::CircularArray< ItemType >;

    enum {
        NICE_LEVEL_CAP = 10, // number of tx which will cause a yield
    };

    alignas(cache_alignment) std::atomic< std::size_t >    top_{ 0 };
    alignas(cache_alignment) std::atomic< std::size_t >    bottom_{ 0 };
    alignas(cache_alignment) std::atomic< ArrayType * >    array_;

    alignas(cache_alignment) std::atomic< Context * >    receiver_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >         closed_{ false };

    std::size_t    nice_sending_{ 0 };

    Spinlock    splk_{};

    char padding_[cacheline_length];

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
    : array_{ new ArrayType{ default_capacity } }
{
}

template<typename T>
AsyncChannel< T >::~AsyncChannel() noexcept
{
    close();

    delete array_;

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
    closed_.store( true, std::memory_order_release );
}

template<typename T>
bool AsyncChannel< T >::is_empty() const noexcept
{
    auto bottom = bottom_.load( std::memory_order_relaxed );
    auto top    = top_.load( std::memory_order_relaxed );
    return bottom <= top;
}

template<typename T>
OpResult AsyncChannel< T >::send( ItemType const & item ) noexcept
{
    return send_impl_( item );
}

template<typename T>
OpResult AsyncChannel< T >::send( ItemType && item ) noexcept
{
    return send_impl_( std::forward< ItemType >( item ) );
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

    auto bottom = bottom_.load( std::memory_order_relaxed );
    auto top    = top_.load( std::memory_order_acquire );
    auto array  = array_.load( std::memory_order_relaxed );

    if (   to_signed( bottom - top )
         > to_signed( array->size() - 1 ) ) {
        auto new_array = array->grow( top, bottom );
        std::swap( array, new_array );
        array_.store( array, std::memory_order_release );
        delete new_array;
    }

    array->put( bottom, std::move( item ) );
    std::atomic_thread_fence( std::memory_order_release );
    bottom_.store( bottom + 1, std::memory_order_relaxed );

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
        auto top = top_.load( std::memory_order_acquire );
        std::atomic_thread_fence( std::memory_order_seq_cst );
        auto bottom = bottom_.load( std::memory_order_acquire );

        // if channel empty, wait for a sender to wake it up
        if ( to_signed( bottom - top ) <= 0 ) {
            if ( is_closed() ) {
                return OpResult::Closed;
            }

            std::unique_lock< Spinlock > lk{ splk_ };
            push_receiver_( running_ctx );
            lk.unlock(); // FIXME: sync unlocking
            Scheduler::self()->resume();
            lk.lock();
            continue;
        }

        auto array = array_.load( std::memory_order_consume );

        if ( top_.compare_exchange_weak(
                top,                        // expected
                top + 1,                    // desired
                std::memory_order_seq_cst,  // success
                std::memory_order_relaxed   // fail
        ) ) {
            item = std::move( array->get( top ) );
            return OpResult::Ok;
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
    { return chan_->send( std::forward< ItemType >( item ) ); }

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

} // namespace async
} // namespace channel
PROXC_NAMESPACE_END

#if defined(PROXC_COMP_CLANG)
#   pragma clang diagnostic pop
#endif
