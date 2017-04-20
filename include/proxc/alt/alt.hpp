
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
    using ChoiceT = alt::ChoiceBase;
    using ChoicePtr = std::unique_ptr< ChoiceT >;

    using ChannelId = channel::detail::ChannelId;

    std::vector< ChoicePtr >                     choices_;
    std::unique_ptr< alt::ChoiceTimeout >        timeout_{ nullptr };

    Context *    ctx_;

    Spinlock splk_;

    alignas(cache_alignment) std::atomic< ChoiceT * >    selected_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >            wakeup_{ false };

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
        SmallVec< ChoiceT *, 4 >    vec_; // FIXME: magic numbers
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
    template<typename Clock, typename Dur>
    PROXC_WARN_UNUSED
    Alt & timeout( std::chrono::time_point< Clock, Dur > const &,
                   alt::ChoiceTimeout::FnT = []{} ) noexcept;

    template<typename Rep, typename Period>
    PROXC_WARN_UNUSED
    Alt & timeout( std::chrono::duration< Rep, Period > const &,
                   alt::ChoiceTimeout::FnT = []{} ) noexcept;

    // timeout with guard
    template<typename Clock, typename Dur>
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      std::chrono::time_point< Clock, Dur > const &,
                      alt::ChoiceTimeout::FnT = []{} ) noexcept;

    template<typename Rep, typename Period>
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      std::chrono::duration< Rep, Period > const &,
                      alt::ChoiceTimeout::FnT = []{} ) noexcept;

    // consumes alt and determines which choice to select.
    // the chosen choice completes the operation, and an
    // optional corresponding closure is executed.
    void select();

private:
    [[noreturn]]
    void      select_0();
    ChoiceT * select_1( ChoiceT * ) noexcept;
    ChoiceT * select_n( std::vector< ChoiceT * > & ) noexcept;

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
        : *this
        ;
}

// timeout without guard
template<typename Clock, typename Dur>
Alt & Alt::timeout(
    std::chrono::time_point< Clock, Dur > const & time_point,
    alt::ChoiceTimeout::FnT fn
) noexcept
{
    if ( ! timeout_ || ! timeout_->is_less( time_point ) ) {
        timeout_.reset( new alt::ChoiceTimeout{ this, time_point, std::move( fn ) } );
    }
    return *this;
}

template<typename Rep, typename Period>
Alt & Alt::timeout(
    std::chrono::duration< Rep, Period > const & duration,
    alt::ChoiceTimeout::FnT fn
) noexcept
{
    return timeout(
        std::chrono::steady_clock::now() + duration,
        std::move( fn )
    );
}

// timeout with guard
template<typename Clock, typename Dur>
Alt & Alt::timeout_if(
    bool guard,
    std::chrono::time_point< Clock, Dur > const & time_point,
    alt::ChoiceTimeout::FnT fn
) noexcept
{
    return ( guard )
        ? timeout( time_point,
                   std::move( fn ) )
        : *this
        ;
}

template<typename Rep, typename Period>
Alt & Alt::timeout_if(
    bool guard,
    std::chrono::duration< Rep, Period > const & duration,
    alt::ChoiceTimeout::FnT fn
) noexcept
{
    return ( guard )
        ? timeout( std::chrono::steady_clock::now() + duration,
                   std::move( fn ) )
        : *this
        ;
}

PROXC_NAMESPACE_END

