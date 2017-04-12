
#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/exceptions.hpp>
#include <proxc/channel/sync.hpp>
#include <proxc/channel/op.hpp>
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
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & send( channel::sync::Tx< ItemType > &,
                ItemType &&,
                typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    // send choice with guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & send_if( bool,
                   channel::sync::Tx< ItemType > &,
                   ItemType &&,
                   typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    // recv choice without guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & recv( channel::sync::Rx< ItemType > &,
                typename alt::ChoiceRecv< ItemType >::FnType && = []( ItemType ){} ) noexcept;

    // recv choice with guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & recv_if( bool,
                   channel::sync::Rx< ItemType > &,
                   typename alt::ChoiceRecv< ItemType >::FnType && = []( ItemType ){} ) noexcept;

    // timeout without guard
    template<typename Clock, typename Dur>
    PROXC_WARN_UNUSED
    Alt & timeout( std::chrono::time_point< Clock, Dur > const &,
                   alt::ChoiceTimeout::FnType = []{} ) noexcept;

    template<typename Rep, typename Period>
    PROXC_WARN_UNUSED
    Alt & timeout( std::chrono::duration< Rep, Period > const &,
                   alt::ChoiceTimeout::FnType = []{} ) noexcept;

    // timeout with guard
    template<typename Clock, typename Dur>
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      std::chrono::time_point< Clock, Dur > const &,
                      alt::ChoiceTimeout::FnType = []{} ) noexcept;

    template<typename Rep, typename Period>
    PROXC_WARN_UNUSED
    Alt & timeout_if( bool,
                      std::chrono::duration< Rep, Period > const &,
                      alt::ChoiceTimeout::FnType = []{} ) noexcept;

    // consumes alt and determines which choice to select
    void select();

private:
    using ChoicePtr = std::unique_ptr< alt::ChoiceBase >;

    std::vector< ChoicePtr >                 choices_;
    std::unique_ptr< alt::ChoiceTimeout >    timeout_{ nullptr };
};

Alt::Alt()
{
    choices_.reserve( 8 );
}

// send choice without guard
template<typename ItemType>
Alt & Alt::send(
    channel::sync::Tx< ItemType > & tx,
    ItemType && item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    choices_.emplace_back(
        new alt::ChoiceSend< ItemType >{
            tx,
            std::forward< ItemType >( item ),
            std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn )
        }
    );
    return *this;
}

// send choice with guard
template<typename ItemType>
Alt & Alt::send_if(
    bool guard,
    channel::sync::Tx< ItemType > & tx,
    ItemType && item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    return ( guard )
        ? send( tx,
                std::forward< ItemType >( item ),
                std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn ) )
        : *this
        ;
}

// recv choice without guard
template<typename ItemType>
Alt & Alt::recv(
    channel::sync::Rx< ItemType > & rx,
    typename alt::ChoiceRecv< ItemType >::FnType && fn
) noexcept
{
    choices_.emplace_back(
        new alt::ChoiceRecv< ItemType >{
            rx,
            std::forward< typename alt::ChoiceRecv< ItemType >::FnType >( fn )
        }
    );
    return *this;
}

// recv choice with guard
template<typename ItemType>
Alt & Alt::recv_if(
    bool guard,
    channel::sync::Rx< ItemType > & rx,
    typename alt::ChoiceRecv< ItemType >::FnType && fn
) noexcept
{
    return ( guard )
        ? recv( rx,
                std::forward< typename alt::ChoiceRecv< ItemType >::FnType >( fn ) )
        : *this
        ;
}

// timeout without guard
template<typename Clock, typename Dur>
Alt & Alt::timeout(
    std::chrono::time_point< Clock, Dur > const & time_point,
    alt::ChoiceTimeout::FnType fn
) noexcept
{
    if ( ! timeout_ || ! timeout_->is_less( time_point ) ) {
        timeout_.reset( new alt::ChoiceTimeout{ time_point, std::move( fn ) } );
    }
    return *this;
}

template<typename Rep, typename Period>
Alt & Alt::timeout(
    std::chrono::duration< Rep, Period > const & duration,
    alt::ChoiceTimeout::FnType fn
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
    alt::ChoiceTimeout::FnType fn
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
    alt::ChoiceTimeout::FnType fn
) noexcept
{
    return ( guard )
        ? timeout( std::chrono::steady_clock::now() + duration,
                   std::move( fn ) )
        : *this
        ;
}

void Alt::select()
{
    // if timeout choice has been set, add to choices
    if ( timeout_ ) {
        choices_.emplace_back( timeout_.release() );
    }

    if ( choices_.empty() ) {
        // suspend indefinitely, should never return
        Scheduler::self()->resume();
        BOOST_ASSERT_MSG( false, "unreachable" );
        throw UnreachableError{};

    } else {
        std::vector< alt::ChoiceBase * > ready_;
        // FIXME: reserve? and if so, at what size?
        ready_.reserve( choices_.size() );
        for ( auto& choice : choices_ ) {
            if ( choice->is_ready() ) {
                ready_.push_back( choice.get() );
            }
        }

        alt::ChoiceBase * selected = nullptr;
        if ( ! ready_.empty() ) {
            // choose one choice immediately
            if ( ready_.size() == 1 ) {
                selected = ready_.front();
            } else {
                static thread_local std::minstd_rand rng;
                auto id = std::uniform_int_distribution< std::size_t >
                    { 0, ready_.size() - 1 }( rng );
                selected = ready_[id];
            }

        } else {
            // wait until one of the choices are available
            for ( auto& choice : choices_ ) {
                (void)choice;
                // activate?
            }
        }

        BOOST_ASSERT( selected != nullptr );

        selected->complete_task();
    }
}

PROXC_NAMESPACE_END


