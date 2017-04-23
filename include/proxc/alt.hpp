
#pragma once

#include <algorithm>
#include <chrono>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/channel.hpp>
#include <proxc/timer.hpp>
#include <proxc/exceptions.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/detail/delegate.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/alt/choice_send.hpp>
#include <proxc/alt/choice_recv.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

} // namespace detail

class Alt
{
private:
    using ChoiceT   = alt::ChoiceBase;
    using ChoicePtr = std::unique_ptr< ChoiceT >;

    using ChannelId = channel::detail::ChannelId;

    template<typename EndT>
    using ItemT = typename EndT::ItemT;
    template<typename EndIt>
    using EndT = typename std::iterator_traits< EndIt >::value_type;

    template<typename Tx>
    using TxFn = detail::delegate< void( void ) >;
    template<typename Rx>
    using RxFn = detail::delegate< void( ItemT< Rx > ) >;

    using TimePointT = Context::TimePointT;
    using TimerFn = detail::delegate< void( void ) >;

    std::vector< ChoicePtr >    choices_{};

    TimePointT      time_point_{ TimePointT::max() };
    TimerFn         timer_fn_{};

    Context *    ctx_;
    Spinlock     splk_;

    alignas(cache_alignment) std::atomic_flag            select_flag_;
    alignas(cache_alignment) std::atomic< ChoiceT * >    selected_{ nullptr };

    template<typename T, std::size_t N>
    using SmallVec = boost::container::small_vector< T, N >;

    struct ChoiceAudit
    {
        enum class State {
            Tx,
            Rx,
            Clash,
        };
        State                       state_;
        SmallVec< ChoiceT *, 4 >    vec_; // FIXME: magic number
        ChoiceAudit() = default;
        ChoiceAudit( State state, ChoiceT * choice )
            : state_{ state }, vec_{ choice }
        {}
    };
    std::map< ChannelId, ChoiceAudit >    ch_audit_;

    friend class alt::ChoiceBase;
    friend class Scheduler;

public:
    Alt();
    ~Alt() {}

    // make non-copyable
    Alt( Alt const & ) = delete;
    Alt & operator = ( Alt const & ) = delete;

    // make non-movable
    Alt( Alt && ) = delete;
    Alt & operator = ( Alt && ) = delete;

    // send choice without guard
    template< typename Tx
            , typename = std::enable_if_t<
                traits::is_tx< Tx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send( Tx & tx,
                ItemT< Tx > && item,
                TxFn< Tx > fn = TxFn< Tx >{} ) noexcept;

    template< typename Tx
            , typename = std::enable_if_t<
                traits::is_tx< Tx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send( Tx & tx,
                ItemT< Tx > const & item,
                TxFn< Tx > fn = TxFn< Tx >{} ) noexcept;

    // send choice with guard
    template< typename Tx
            , typename = std::enable_if_t<
                traits::is_tx< Tx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send_if( bool guard,
                   Tx & tx,
                   ItemT< Tx > && item,
                   TxFn< Tx > fn = TxFn< Tx >{} ) noexcept;

    template< typename Tx
            , typename = std::enable_if_t<
                traits::is_tx< Tx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send_if( bool guard,
                   Tx & tx,
                   ItemT< Tx > const & item,
                   TxFn< Tx > fn = TxFn< Tx >{} ) noexcept;

    // replicated send choice over item iterator
    template< typename TxIt, typename ItemIt
            , typename = std::enable_if_t<
                traits::is_tx_iterator< TxIt >::value &&
                traits::is_inputiterator< ItemIt >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send_for( TxIt tx_first, TxIt tx_last,
                    ItemIt item_first,
                    TxFn< EndT< TxIt > > fn = TxFn< EndT< TxIt > >{} ) noexcept;

    // replicated send choice over single item
    template< typename TxIt
            , typename = std::enable_if_t<
                traits::is_tx_iterator< TxIt >::value
            > >
    PROXC_WARN_UNUSED
    Alt & send_for( TxIt tx_first, TxIt tx_last,
                    ItemT< EndT< TxIt > > item,
                    TxFn< EndT< TxIt > > fn = TxFn< EndT< TxIt > >{} ) noexcept;

    // recv choice without guard
    template< typename Rx
            , typename = std::enable_if_t<
                traits::is_rx< Rx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & recv( Rx & rx,
                RxFn< Rx > fn = RxFn< Rx >{} ) noexcept;

    // recv choice with guard
    template< typename Rx
            , typename = std::enable_if_t<
                traits::is_rx< Rx >::value
            > >
    PROXC_WARN_UNUSED
    Alt & recv_if( bool guard,
                   Rx & rx,
                   RxFn< Rx > fn = RxFn< Rx >{} ) noexcept;

    // replicated recv choice
    template< typename RxIt
            , typename = std::enable_if_t<
                traits::is_rx_iterator< RxIt >::value
            > >
    PROXC_WARN_UNUSED
    Alt & recv_for( RxIt rx_first, RxIt rx_last,
                    RxFn< EndT< RxIt > > fn = RxFn< EndT< RxIt > >{} ) noexcept;

    // timeout without guard
    template< typename Timer
            , typename = typename std::enable_if<
                traits::is_timer< Timer >::value
            >::type >
    PROXC_WARN_UNUSED
    Alt & timeout( Timer const &,
                   TimerFn fn = TimerFn{} ) noexcept;

    // timeout with guard
    template< typename Timer
            , typename = typename std::enable_if<
                traits::is_timer< Timer >::value
            >::type >
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      Timer const &,
                      TimerFn = TimerFn{} ) noexcept;

    // consumes alt and determines which choice to select.
    // the chosen choice completes the operation, and an
    // optional corresponding closure is executed.
    void select();

private:
    bool select_0();
    bool select_1( ChoiceT * ) noexcept;
    bool select_n( std::vector< ChoiceT * > & ) noexcept;

    bool try_select( ChoiceT * ) noexcept;
    bool try_timeout() noexcept;
    void maybe_wakeup() noexcept;
};

// send choice without guard
template<typename Tx, typename>
Alt & Alt::send(
    Tx & tx,
    ItemT< Tx > && item,
    TxFn< Tx > fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        ChannelId id = tx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Tx ) {
            auto pc = std::make_unique< alt::ChoiceSend< ItemT< Tx > > >(
                this,
                ctx_,
                tx,
                std::move( item ),
                std::move( fn )
            );
            if ( audit_it == ch_audit_.end() ) {
                ch_audit_[ id ] = ChoiceAudit{
                    ChoiceAudit::State::Tx,
                    pc.get()
                };
            } else { // state == State::Tx;
                ch_audit_[ id ].vec_.push_back( pc.get() );
            }
            choices_.push_back( std::move( pc ) );
        } else {
            ch_audit_[ id ].state_ = ChoiceAudit::State::Clash;
        }
    }
    return *this;
}

template<typename Tx, typename>
Alt & Alt::send(
    Tx & tx,
    ItemT< Tx > const & item,
    TxFn< Tx > fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        ChannelId id = tx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Tx ) {
            auto pc = std::make_unique< alt::ChoiceSend< ItemT< Tx > > >(
                this,
                ctx_,
                tx,
                item,
                std::move( fn )
            );
            if ( audit_it == ch_audit_.end() ) {
                ch_audit_[ id ] = ChoiceAudit{
                    ChoiceAudit::State::Tx,
                    pc.get()
                };
            } else { // state == State::Tx;
                ch_audit_[ id ].vec_.push_back( pc.get() );
            }
            choices_.push_back( std::move( pc ) );
        } else {
            ch_audit_[ id ].state_ = ChoiceAudit::State::Clash;
        }
    }
    return *this;
}

// send choice with guard
template<typename Tx, typename>
Alt & Alt::send_if(
    bool guard,
    Tx & tx,
    ItemT< Tx > && item,
    TxFn< Tx > fn
) noexcept
{
    return ( guard )
        ? send( tx,
                std::move( item ),
                std::move( fn ) )
        : *this
        ;
}

template<typename Tx, typename>
Alt & Alt::send_if(
    bool guard,
    Tx & tx,
    ItemT< Tx > const & item,
    TxFn< Tx > fn
) noexcept
{
    return ( guard )
        ? send( tx,
                item,
                std::move( fn ) )
        : *this
        ;
}

// replicated send choice over item iterator
template<typename TxIt, typename ItemIt, typename>
Alt & Alt::send_for(
    TxIt   tx_first,   TxIt   tx_last,
    ItemIt item_first,
    TxFn< EndT< TxIt > > fn
) noexcept
{
    for ( auto tx_it = tx_first;
          tx_it != tx_last;
          ++tx_it ) {
        (void)send( *tx_it, *item_first++, fn );
    }
    return *this;
}

// replicated send choice over single item
template<typename TxIt, typename>
Alt & Alt::send_for(
    TxIt tx_first, TxIt tx_last,
    ItemT< EndT< TxIt > > item,
    TxFn< EndT< TxIt > > fn
) noexcept
{
    for ( auto tx_it = tx_first;
          tx_it != tx_last;
          ++tx_it ) {
        (void)send( *tx_it, item, fn );
    }
    return *this;
}

// recv choice without guard
template<typename Rx, typename>
Alt & Alt::recv(
    Rx & rx,
    RxFn< Rx > fn
) noexcept
{
    if ( ! rx.is_closed() ) {
        ChannelId id = rx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Rx ) {
            auto pc = std::make_unique< alt::ChoiceRecv< ItemT< Rx > > >(
                this,
                ctx_,
                rx,
                std::move( fn )
            );
            if ( audit_it == ch_audit_.end() ) {
                ch_audit_[ id ] = ChoiceAudit{
                    ChoiceAudit::State::Rx,
                    pc.get()
                };
            } else { // state == State::Rx;
                ch_audit_[ id ].vec_.push_back( pc.get() );
            }
            choices_.push_back( std::move( pc ) );
        } else {
            ch_audit_[ id ].state_ = ChoiceAudit::State::Clash;
        }
    }
    return *this;
}

// recv choice with guard
template<typename Rx, typename>
Alt & Alt::recv_if(
    bool guard,
    Rx & rx,
    RxFn< Rx > fn
) noexcept
{
    return ( guard )
        ? recv( rx,
                std::move( fn ) )
        : *this ;
}

// replicated recv choice
template<typename RxIt, typename>
Alt & Alt::recv_for(
    RxIt rx_first, RxIt rx_last,
    RxFn< EndT< RxIt > > fn
) noexcept
{
    for ( auto rx_it = rx_first;
          rx_it != rx_last;
          ++rx_it ) {
        (void)recv( *rx_it, fn );
    }
    return *this;
}

// timeout without guard
template< typename Timer
        , typename >
Alt & Alt::timeout(
    Timer const & tmi,
    TimerFn fn
) noexcept
{
    Timer new_timer{ tmi };
    new_timer.reset();
    if ( new_timer.get() < time_point_ ) {
        time_point_ = new_timer.get();
        timer_fn_ = std::move( fn );
    }
    return *this;
}

// timeout with guard
template< typename Timer
        , typename >
Alt & Alt::timeout_if(
    bool guard,
    Timer const & tmi,
    TimerFn fn
) noexcept
{
    return ( guard )
        ? timeout( tmi, std::move( fn ) )
        : *this ;
}

PROXC_NAMESPACE_END
