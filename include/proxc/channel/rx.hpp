
#pragma once

#include <iterator>
#include <memory>
#include <tuple>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
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

template<typename T>
class Rx
{
public:
    using ItemType = T;

private:
    using ChanType = detail::ChannelImpl< ItemType >;
    using ChanPtr = std::shared_ptr< ChanType >;

    ChanPtr    chan_{ nullptr };

public:
    Rx() = default;
    ~Rx()
    {
        if ( chan_ ) {
            chan_->close();
        }
    }

    // make non-copyable
    Rx( Rx const & )               = delete;
    Rx & operator = ( Rx const & ) = delete;

    // make movable
    Rx( Rx && )               = default;
    Rx & operator = ( Rx && ) = default;

    bool is_closed() const noexcept
    {
        return chan_->is_closed();
    }

    void close() noexcept
    {
        chan_->close();
        chan_.reset();
    }

    // normal recv operations
    OpResult recv( ItemType & item ) noexcept
    {
        ChanEnd rx{ Scheduler::running() };
        return chan_->recv( rx, item );
    }

    // recv operation with duration timeout
    template<typename Rep, typename Period>
    OpResult recv_for( ItemType & item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return recv_until( item, time_point );
    }

    // recv operation with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult recv_until( ItemType & item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        ChanEnd rx{ Scheduler::running() };
        return chan_->recv_until( rx, item, time_point );
    }

private:
    Rx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

    friend class ::proxc::alt::ChoiceRecv< T >;

    void alt_enter( ChanEnd & rx ) noexcept
    {
        chan_->alt_recv_enter( rx );
    }

    void alt_leave() noexcept
    {
        chan_->alt_recv_leave();
    }

    bool alt_ready( Alt * alt ) const noexcept
    {
        return chan_->alt_recv_ready( alt );
    }

    AltResult alt_recv( ItemType & item ) noexcept
    {
        return chan_->alt_recv( item );
    }

public:
    class Iterator
        : public std::iterator<
            std::input_iterator_tag,
            typename std::remove_reference< ItemType >::type
          >
    {
    private:
        using StorageType = typename std::aligned_storage<
            sizeof( ItemType ), alignof( ItemType )
        >::type;

        Rx< T > *      rx_{ nullptr };
        StorageType    storage_;

        void increment() noexcept
        {
            BOOST_ASSERT( rx_ != nullptr );
            ItemType item{};
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

} // namespace channel
PROXC_NAMESPACE_END


