
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

template<typename ItemT>
class Rx
{
public:
    using Id = detail::ChannelId;

private:
    using ChanT = detail::ChannelImpl< ItemT >;
    using EndT  = detail::ChanEnd< ItemT >;
    using ChanPtr = std::shared_ptr< ChanT >;

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

    // normal recv operations
    OpResult recv( ItemT & item ) noexcept
    {
        EndT rx{ Scheduler::running(), item };
        return chan_->recv( rx );
    }

    // recv operation with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult recv_until( ItemT & item,
                         std::chrono::time_point< Clock, Dur > const & time_point
    ) noexcept
    {
        EndT rx{ Scheduler::running(), item };
        return chan_->recv_until( rx, time_point );
    }

    // recv operation with duration timeout
    template<typename Rep, typename Period>
    OpResult recv_for( ItemT & item,
                       std::chrono::duration< Rep, Period > const & duration
    ) noexcept
    {
        auto time_point = std::chrono::steady_clock::now() + duration;
        return recv_until( item, time_point );
    }


private:
    Rx( ChanPtr ptr )
        : chan_{ ptr }
    {}

    template<typename U>
    friend std::tuple< Tx< U >, Rx< U > >
    create() noexcept;

    friend class ::proxc::alt::ChoiceRecv< ItemT >;

    void alt_enter( EndT & rx ) noexcept
    {
        chan_->alt_recv_enter( rx );
    }

    void alt_leave() noexcept
    {
        chan_->alt_recv_leave();
    }

    bool alt_ready() const noexcept
    {
        return chan_->alt_recv_ready();
    }

    AltResult alt_recv() noexcept
    {
        return chan_->alt_recv();
    }

public:
    class Iterator
        : public std::iterator<
            std::input_iterator_tag,
            typename std::remove_reference< ItemT >::type
          >
    {
    private:
        using StorageT = typename std::aligned_storage<
            sizeof( ItemT ), alignof( ItemT )
        >::type;
        using RxT = Rx< ItemT >;

        RxT *       rx_{ nullptr };
        StorageT    storage_;

        void increment() noexcept
        {
            BOOST_ASSERT( rx_ != nullptr );
            ItemT item{};
            auto res = rx_->recv( item );
            if ( res == OpResult::Ok ) {
                auto addr = static_cast< void * >( std::addressof( storage_ ) );
                ::new (addr) ItemT{ std::move( item ) };
            } else {
                rx_ = nullptr;
            }
        }

    public:
        using Ptr = typename Iterator::pointer;
        using Ref = typename Iterator::reference;

        Iterator() noexcept = default;

        explicit Iterator( Rx< ItemT > * rx ) noexcept
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
        { return * reinterpret_cast< ItemT * >( std::addressof( storage_ ) ); }

        Ptr operator -> () noexcept
        { return reinterpret_cast< ItemT * >( std::addressof( storage_ ) ); }
    };
};

template<typename ItemT>
typename Rx< ItemT >::Iterator
begin( Rx< ItemT > & chan )
{
    return typename Rx< ItemT >::Iterator( & chan );
}

template<typename ItemT>
typename Rx< ItemT >::Iterator
end( Rx< ItemT > & )
{
    return typename Rx< ItemT >::Iterator();
}

} // namespace channel
PROXC_NAMESPACE_END


