
#pragma once

#include <iterator>
#include <memory>
#include <tuple>

#include <proxc/config.hpp>

#include <proxc/channel/sync.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {

template<typename T>
class Rx
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


} // namespace channel
PROXC_NAMESPACE_END


