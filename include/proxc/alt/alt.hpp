
#pragma once

#include <algorithm>
#include <chrono>
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

class Alt
{
private:
    using ChoiceType = alt::ChoiceBase;
    using ChoicePtr = std::unique_ptr< ChoiceType >;

    std::vector< ChoicePtr >                     choices_;
    std::unique_ptr< alt::ChoiceTimeout >        timeout_{ nullptr };

<<<<<<< HEAD
    Context *    ctx_;

    Spinlock splk_;

    alignas(cache_alignment) std::atomic< ChoiceType * >    selected_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >            wakeup_{ false };

=======
<<<<<<< HEAD
    Context *    alt_ctx_;

    Spinlock splk_;

    alignas(cache_alignment) std::atomic< alt::ChoiceBase * >    selected_{ nullptr };
    alignas(cache_alignment) std::atomic< Context * >            wakeup_{ nullptr };

    friend class ::proxc::Scheduler;
=======
    Context *    ctx_;

    Spinlock splk_;

    alignas(cache_alignment) std::atomic< ChoiceType * >    selected_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >            wakeup_{ false };

>>>>>>> origin/diploma
>>>>>>> merge
    friend class alt::ChoiceBase;

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
    Alt & send( channel::Tx< ItemType > &,
                ItemType &&,
                typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & send( channel::Tx< ItemType > &,
                ItemType const &,
                typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    // send choice with guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & send_if( bool,
                   channel::Tx< ItemType > &,
                   ItemType &&,
                   typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & send_if( bool,
                   channel::Tx< ItemType > &,
                   ItemType const &,
                   typename alt::ChoiceSend< ItemType >::FnType && = []{} ) noexcept;

    // recv choice without guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & recv( channel::Rx< ItemType > &,
                typename alt::ChoiceRecv< ItemType >::FnType && = []( ItemType ){} ) noexcept;

    // recv choice with guard
    template<typename ItemType>
    PROXC_WARN_UNUSED
    Alt & recv_if( bool,
                   channel::Rx< ItemType > &,
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
    [[noreturn]]
    void select_0();
    ChoiceType * select_1() noexcept;
    ChoiceType * select_n() noexcept;

<<<<<<< HEAD
    void wait( std::unique_lock< Spinlock > & ) noexcept;
    bool try_select( ChoiceType * ) noexcept;
=======
<<<<<<< HEAD
    void wait( std::unique_lock< Spinlock > * ) noexcept;
    bool try_select( alt::ChoiceBase * ) noexcept;
=======
    void wait( std::unique_lock< Spinlock > & ) noexcept;
    bool try_select( ChoiceType * ) noexcept;
>>>>>>> origin/diploma
>>>>>>> merge
    void maybe_wakeup() noexcept;
};

Alt::Alt()
<<<<<<< HEAD
    : ctx_{ Scheduler::running() }
=======
<<<<<<< HEAD
    : alt_ctx_{ Scheduler::running() }
=======
    : ctx_{ Scheduler::running() }
>>>>>>> origin/diploma
>>>>>>> merge
{
    choices_.reserve( 8 );
}

// send choice without guard
template<typename ItemType>
Alt & Alt::send(
    channel::Tx< ItemType > & tx,
    ItemType && item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        choices_.push_back(
            std::make_unique< alt::ChoiceSend< ItemType > >(
                this,
<<<<<<< HEAD
                ctx_,
=======
<<<<<<< HEAD
                alt_ctx_,
=======
                ctx_,
>>>>>>> origin/diploma
>>>>>>> merge
                tx,
                std::move( item ),
                std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn )
            ) );
    }
    return *this;
}

template<typename ItemType>
Alt & Alt::send(
    channel::Tx< ItemType > & tx,
    ItemType const & item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    if ( ! tx.is_closed() ) {
        choices_.push_back(
            std::make_unique< alt::ChoiceSend< ItemType > >(
                this,
<<<<<<< HEAD
                ctx_,
=======
<<<<<<< HEAD
                alt_ctx_,
=======
                ctx_,
>>>>>>> origin/diploma
>>>>>>> merge
                tx,
                item,
                std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn )
            ) );
    }
    return *this;
}

// send choice with guard
template<typename ItemType>
Alt & Alt::send_if(
    bool guard,
    channel::Tx< ItemType > & tx,
    ItemType && item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    return ( guard )
        ? send( tx,
                std::move( item ),
                std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn ) )
        : *this
        ;
}

template<typename ItemType>
Alt & Alt::send_if(
    bool guard,
    channel::Tx< ItemType > & tx,
    ItemType const & item,
    typename alt::ChoiceSend< ItemType >::FnType && fn
) noexcept
{
    return ( guard )
        ? send( tx,
                item,
                std::forward< typename alt::ChoiceSend< ItemType >::FnType >( fn ) )
        : *this
        ;
}

// recv choice without guard
template<typename ItemType>
Alt & Alt::recv(
    channel::Rx< ItemType > & rx,
    typename alt::ChoiceRecv< ItemType >::FnType && fn
) noexcept
{
    if ( ! rx.is_closed() ) {
        choices_.push_back(
            std::make_unique< alt::ChoiceRecv< ItemType > >(
                this,
<<<<<<< HEAD
                ctx_,
=======
<<<<<<< HEAD
                alt_ctx_,
=======
                ctx_,
>>>>>>> origin/diploma
>>>>>>> merge
                rx,
                std::forward< typename alt::ChoiceRecv< ItemType >::FnType >( fn )
            ) );
    }
    return *this;
}

// recv choice with guard
template<typename ItemType>
Alt & Alt::recv_if(
    bool guard,
    channel::Rx< ItemType > & rx,
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
        timeout_.reset( new alt::ChoiceTimeout{ this, time_point, std::move( fn ) } );
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

    for ( auto& choice : choices_ ) {
        choice->enter();
    }

    alt::ChoiceBase * selected;
    switch ( choices_.size() ) {
    case 0:  select_0(); // never returns
    case 1:  selected = select_1(); break;
    default: selected = select_n(); break;
    }

    BOOST_ASSERT( selected != nullptr );
    selected->run_func();
}

void Alt::select_0()
{
    // suspend indefinitely, should never return
    Scheduler::self()->wait();
    BOOST_ASSERT_MSG( false, "unreachable" );
    throw UnreachableError{};
}

auto Alt::select_1() noexcept
    -> ChoiceType *
{
    std::unique_lock< Spinlock > lk{ splk_ };
<<<<<<< HEAD

    ChoiceType * choice = choices_.begin()->get();

=======
<<<<<<< HEAD
    auto choice = choices_.begin()->get();

    choice->enter();

    for ( ;; ) {
        if ( ! choice->is_ready() ) {
            wait( & lk );
            // woken up, case should be ready
=======

    ChoiceType * choice = choices_.begin()->get();

>>>>>>> merge
    choice->enter();
    do {
        if ( ! choice->is_ready() ) {
            wait( lk );
            // choice should be ready now
            continue;
>>>>>>> origin/diploma
        }
    } while ( choice->try_complete() );
    choice->leave();

<<<<<<< HEAD
    return choice;
}

auto Alt::select_n() noexcept
    -> ChoiceType *
=======
<<<<<<< HEAD
        if ( choice->try_complete() ) {
            choice->leave();
            return choice;
        }
    }
}

alt::ChoiceBase * Alt::select_n() noexcept
=======
    return choice;
}

auto Alt::select_n() noexcept
    -> ChoiceType *
>>>>>>> origin/diploma
>>>>>>> merge
{
    std::vector< ChoiceType * > ready;
    // FIXME: reserve? and if so, at what size?
    ready.reserve( choices_.size() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for ( auto& choice : choices_ ) {
        choice->enter();
    }

<<<<<<< HEAD
    ChoiceType * selected = nullptr;
    while ( selected == nullptr ) {
=======
<<<<<<< HEAD
    alt::ChoiceBase * selected = nullptr;
    while( selected == nullptr ) {
=======
    ChoiceType * selected = nullptr;
    while ( selected == nullptr ) {
>>>>>>> origin/diploma
>>>>>>> merge
        ready.clear();
        for ( auto& choice : choices_ ) {
            if ( choice->is_ready() ) {
                ready.push_back( choice.get() );
            }
        }

        if ( ready.empty() ) {
<<<<<<< HEAD
=======
<<<<<<< HEAD
            wait( & lk );

            // woken up, selected should be set
            selected = selected_.load( std::memory_order_acquire );
            if ( selected != nullptr && selected->try_complete() ) {
                continue;
            }
            selected_.store( nullptr, std::memory_order_release );
            selected = nullptr;
            // failed completing the alt case, usually because
            // the other end was alting and failed to set selected.

=======
>>>>>>> merge
            wait( lk );
            // one or more choices should be ready,
            // and selected_ should be set. If set,
            // this is the winning choice.
            selected = selected_.load( std::memory_order_acquire );
            if ( selected != nullptr && ! selected->try_complete() ) {
                // selected was set, but failed to compelte the transaction.
                // check ready cases again and reset selected.
                selected_.store( nullptr, std::memory_order_release );
                selected = nullptr;
            }
            continue;
        }

        if ( ready.size() == 1 ) {
            selected = *ready.begin();
            if ( ! selected->try_complete() ) {
                selected = nullptr;
            }
<<<<<<< HEAD
=======
>>>>>>> origin/diploma
>>>>>>> merge
        } else {
            static thread_local std::mt19937 rng{ std::random_device{}() };
            std::shuffle( ready.begin(), ready.end(), rng );

            for ( auto& choice : ready ) {
                if ( choice->try_complete() ) {
                    selected = choice;
                    break;
                }
            }
        }
    }

    for ( auto& choice : choices_ ) {
        choice->leave();
    }
<<<<<<< HEAD

=======
<<<<<<< HEAD
=======

>>>>>>> origin/diploma
>>>>>>> merge
    selected_.store( nullptr, std::memory_order_release );
    return selected;
}

<<<<<<< HEAD
void Alt::wait( std::unique_lock< Spinlock > & lk ) noexcept
{
=======
<<<<<<< HEAD
void Alt::wait( std::unique_lock< Spinlock > * lk ) noexcept
{
    wakeup_.store( end_.ctx_, std::memory_order_release );
    Scheduler::self()->wait( lk, true );
    wakeup_.store( nullptr, std::memory_order_release );
}

// called externally by choices
bool Alt::try_select( alt::ChoiceBase * choice ) noexcept
{
    BOOST_ASSERT( choice != nullptr );

    std::unique_lock< Spinlock > lk{ splk_, std::defer_lock };
    if ( ! lk.try_lock() )  {
        return false;
    }

    alt::ChoiceBase * expected = nullptr;
    bool success = selected_.compare_exchange_strong(
        expected,                       // expected
        choice,                         // desired
        std::memory_order_acq_rel,      // success order
        std::memory_order_relaxed       // fail order
    );

    auto ctx = wakeup_.exchange( nullptr, std::memory_order_acq_rel );
    if ( ctx != nullptr ) {
        Scheduler::self()->schedule( ctx );
    }

=======
void Alt::wait( std::unique_lock< Spinlock > & lk ) noexcept
{
>>>>>>> merge
    wakeup_.store( true, std::memory_order_release );
    Scheduler::self()->wait( std::addressof( lk ), true );
    wakeup_.store( false, std::memory_order_release );
}

// called by external choices
bool Alt::try_select( ChoiceType * choice ) noexcept
{
    std::unique_lock< Spinlock > lk{ splk_, std::defer_lock };
    if ( ! lk.try_lock() ) {
        return false;
    }

    ChoiceType * expected = nullptr;
    bool success = selected_.compare_exchange_strong(
        expected,
        choice,
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    );
    if ( wakeup_.exchange( false, std::memory_order_acq_rel ) ) {
        Scheduler::self()->schedule( ctx_ );
    }
<<<<<<< HEAD
=======
>>>>>>> origin/diploma
>>>>>>> merge
    return success;
}

// called by external choices
void Alt::maybe_wakeup() noexcept
{
<<<<<<< HEAD
    // FIXME: do i need a spinlock?
    /* std::unique_lock< Spinlock > lk{ splk_ }; */

    if ( wakeup_.exchange( false, std::memory_order_acq_rel ) ) {
        Scheduler::self()->schedule( ctx_ );
=======
<<<<<<< HEAD
    std::unique_lock< Spinlock > lk{ splk_, std::defer_lock };

    if ( ! lk.try_lock() )  {
        return;
    }

    auto ctx = wakeup_.exchange( nullptr, std::memory_order_acq_rel );
    if ( ctx != nullptr ) {
        Scheduler::self()->schedule( ctx );
=======
    // FIXME: do i need a spinlock?
    /* std::unique_lock< Spinlock > lk{ splk_ }; */

    if ( wakeup_.exchange( false, std::memory_order_acq_rel ) ) {
        Scheduler::self()->schedule( ctx_ );
>>>>>>> origin/diploma
>>>>>>> merge
    }
}

PROXC_NAMESPACE_END

