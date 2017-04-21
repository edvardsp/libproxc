
#pragma once

#include <algorithm>
#include <chrono>
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
#include <proxc/alt/choice_timeout.hpp>

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

    using TimePointT = Context::TimePointT;
    using TimerFnT   = detail::delegate< void( void ) >;

    std::vector< ChoicePtr >    choices_{};

    TimePointT      time_point_{ TimePointT::max() };
    TimerFnT        timer_fn_{ []{} };

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
    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & send( channel::Tx< ItemT > &,
                ItemT &&,
                typename alt::ChoiceSend< ItemT >::FnT && = []{} ) noexcept;

    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & send( channel::Tx< ItemT > &,
                ItemT const &,
                typename alt::ChoiceSend< ItemT >::FnT && = []{} ) noexcept;

    // send choice with guard
    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & send_if( bool,
                   channel::Tx< ItemT > &,
                   ItemT &&,
                   typename alt::ChoiceSend< ItemT >::FnT && = []{} ) noexcept;

    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & send_if( bool,
                   channel::Tx< ItemT > &,
                   ItemT const &,
                   typename alt::ChoiceSend< ItemT >::FnT && = []{} ) noexcept;

    // recv choice without guard
    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & recv( channel::Rx< ItemT > &,
                typename alt::ChoiceRecv< ItemT >::FnT && = []( ItemT ){} ) noexcept;

    // recv choice with guard
    template<typename ItemT>
    PROXC_WARN_UNUSED
    Alt & recv_if( bool,
                   channel::Rx< ItemT > &,
                   typename alt::ChoiceRecv< ItemT >::FnT && = []( ItemT ){} ) noexcept;

    // timeout without guard
    template< typename Timer
            , typename = typename std::enable_if<
                traits::is_timer< Timer >::value
            >::type >
    PROXC_WARN_UNUSED
    Alt & timeout( Timer const &,
                   TimerFnT = []{} ) noexcept;

    // timeout with guard
    template< typename Timer
            , typename = typename std::enable_if<
                traits::is_timer< Timer >::value
            >::type >
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      Timer const &,
                      TimerFnT = []{} ) noexcept;

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
template<typename ItemT>
Alt & Alt::send(
    channel::Tx< ItemT > & tx,
    ItemT && item,
    typename alt::ChoiceSend< ItemT >::FnT && fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        ChannelId id = tx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Tx ) {
            auto pc = std::make_unique< alt::ChoiceSend< ItemT > >(
                this,
                ctx_,
                tx,
                std::move( item ),
                std::forward< typename alt::ChoiceSend< ItemT >::FnT >( fn )
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

template<typename ItemT>
Alt & Alt::send(
    channel::Tx< ItemT > & tx,
    ItemT const & item,
    typename alt::ChoiceSend< ItemT >::FnT && fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        ChannelId id = tx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Tx ) {
            auto pc = std::make_unique< alt::ChoiceSend< ItemT > >(
                this,
                ctx_,
                tx,
                std::move( item ),
                std::forward< typename alt::ChoiceSend< ItemT >::FnT >( fn )
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
template<typename ItemT>
Alt & Alt::send_if(
    bool guard,
    channel::Tx< ItemT > & tx,
    ItemT && item,
    typename alt::ChoiceSend< ItemT >::FnT && fn
) noexcept
{
    return ( guard )
        ? send( tx,
                std::move( item ),
                std::forward< typename alt::ChoiceSend< ItemT >::FnT >( fn ) )
        : *this
        ;
}

template<typename ItemT>
Alt & Alt::send_if(
    bool guard,
    channel::Tx< ItemT > & tx,
    ItemT const & item,
    typename alt::ChoiceSend< ItemT >::FnT && fn
) noexcept
{
    return ( guard )
        ? send( tx,
                item,
                std::forward< typename alt::ChoiceSend< ItemT >::FnT >( fn ) )
        : *this
        ;
}

// recv choice without guard
template<typename ItemT>
Alt & Alt::recv(
    channel::Rx< ItemT > & rx,
    typename alt::ChoiceRecv< ItemT >::FnT && fn
) noexcept
{
    if ( ! rx.is_closed() ) {
        ChannelId id = rx.get_id();
        auto audit_it = ch_audit_.find( id );
        if ( audit_it == ch_audit_.end()
            || audit_it->second.state_ == ChoiceAudit::State::Rx ) {
            auto pc = std::make_unique< alt::ChoiceRecv< ItemT > >(
                this,
                ctx_,
                rx,
                std::forward< typename alt::ChoiceRecv< ItemT >::FnT >( fn )
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
template<typename ItemT>
Alt & Alt::recv_if(
    bool guard,
    channel::Rx< ItemT > & rx,
    typename alt::ChoiceRecv< ItemT >::FnT && fn
) noexcept
{
    return ( guard )
        ? recv( rx,
                std::forward< typename alt::ChoiceRecv< ItemT >::FnT >( fn ) )
        : *this ;
}

// timeout without guard
template< typename Timer
        , typename >
Alt & Alt::timeout(
    Timer const & tmi,
    TimerFnT fn
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
    TimerFnT fn
) noexcept
{
    return ( guard )
        ? timeout( tmi, std::move( fn ) )
        : *this ;
}

PROXC_NAMESPACE_END

