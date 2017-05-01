/*
 * MIT License
 *
 * Copyright (c) [2017] [Edvard S. Pettersen] <edvard.pettersen@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <deque>
#include <memory>
#include <random>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/runtime/scheduler.hpp>
#include <proxc/alt.hpp>
#include <proxc/alt/sync.hpp>
#include <proxc/alt/choice_base.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Alt impl
////////////////////////////////////////////////////////////////////////////////

Alt::Alt()
    : ctx_{ runtime::Scheduler::running() }
{
    choices_.reserve( 8 );
    select_flag_.clear( std::memory_order_relaxed );
}

void Alt::select()
{
    std::vector< ChoiceT * > choices;
    for ( const auto& kv : ch_audit_ ) {
        auto& audit = kv.second;
        if ( audit.state_ != ChoiceAudit::State::Clash ) {
            static thread_local std::minstd_rand rng{ std::random_device{}() };
            auto size = audit.vec_.size();
            auto ind = ( size > 1 )
                ? std::uniform_int_distribution< std::size_t >{ 0, size-1 }( rng )
                : 0 ;
            choices.push_back( audit.vec_[ ind ] );
        }
    }

    bool timeout = false;
    switch ( choices.size() ) {
    case 0:  timeout = select_0();                   break;
    case 1:  timeout = select_1( *choices.begin() ); break;
    default: timeout = select_n( choices );          break;
    }

    if ( timeout ) {
        BOOST_ASSERT( time_point_ < TimePointT::max() );
        if ( timer_fn_ ) {
            timer_fn_();
        }

    } else {
        auto selected = selected_.exchange( nullptr, std::memory_order_acq_rel );
        BOOST_ASSERT( selected != nullptr );
        selected->run_func();
    }
}

bool Alt::select_0()
{
    if ( time_point_ < TimePointT::max() ) {
        runtime::Scheduler::self()->wait_until( time_point_ );
        return true;

    } else {
        // suspend indefinitely, should never return
        runtime::Scheduler::self()->wait();
        BOOST_ASSERT_MSG( false, "unreachable" );
        throw UnreachableError{};
    }
}

bool Alt::select_1( ChoiceT * choice ) noexcept
{
    std::unique_lock< LockT > lk{ splk_ };

    choice->enter();

    while ( choice->is_ready() ) {
        switch ( choice->try_complete() ) {
        case ChoiceT::Result::TryLater:
            continue;
        case ChoiceT::Result::Ok:
            select_flag_.test_and_set( std::memory_order_relaxed );
            selected_.store( choice, std::memory_order_release );
            /* [[fallthrough]]; */
        case ChoiceT::Result::Failed:
            break;
        }
        break;
    }

    if ( selected_.load( std::memory_order_acquire ) == nullptr ) {
        state_.store( alt::State::Waiting, std::memory_order_release );
        runtime::Scheduler::self()->alt_wait( this, lk );
        state_.store( alt::State::Done, std::memory_order_release );
    } else {
        state_.store( alt::State::Done, std::memory_order_release );
        lk.unlock();
    }

    /* state_.store( alt::State::Done, std::memory_order_release ); */
    /* lk.unlock(); */

    choice->leave();

    return selected_.load( std::memory_order_relaxed ) == nullptr;
}

bool Alt::select_n( std::vector< ChoiceT * > & choices ) noexcept
{
    std::unique_lock< LockT > lk{ splk_ };

    for ( const auto& choice : choices ) {
        choice->enter();
    }

    while ( selected_.load( std::memory_order_acquire ) == nullptr ) {
        std::vector< ChoiceT * > ready;
        for ( const auto& choice : choices ) {
            if ( choice->is_ready() ) {
                ready.push_back( choice );
            }
        }

        const auto size = ready.size();
        if ( size == 0 ) {
            break;
        }
        if ( size > 1 ) {
            static thread_local std::minstd_rand rng{ std::random_device{}() };
            std::shuffle( ready.begin(), ready.end(), rng );
        }

        for ( const auto& choice : ready ) {
            auto res = choice->try_complete();
            if ( res == ChoiceT::Result::Ok ) {
                select_flag_.test_and_set( std::memory_order_relaxed );
                selected_.store( choice, std::memory_order_relaxed );
                break;
            }
        }
    }

    if ( selected_.load( std::memory_order_relaxed ) == nullptr ) {
        state_.store( alt::State::Waiting, std::memory_order_release );
        runtime::Scheduler::self()->alt_wait( this, lk );
        // one or more choices should be ready,
        // and selected_ should be set. If set,
        // this is the winning choice.
        state_.store( alt::State::Done, std::memory_order_release );
    } else {
        state_.store( alt::State::Done, std::memory_order_release );
        lk.unlock();
    }

    /* state_.store( alt::State::Done, std::memory_order_release ); */

    /* lk.unlock(); */

    for ( const auto& choice : choices ) {
        choice->leave();
    }
    return selected_.load( std::memory_order_relaxed ) == nullptr;
}

// called by external non-alting choices
bool Alt::try_select( ChoiceT * choice ) noexcept
{
    std::unique_lock< LockT > lk{ splk_ };

    if ( select_flag_.test_and_set( std::memory_order_acq_rel ) ) {
        return false;
    }

    selected_.store( choice, std::memory_order_release );
    return true;
}

// called by external alting choices
bool Alt::try_alt_select( ChoiceT * choice ) noexcept
{
    BOOST_ASSERT( choice != nullptr );

    std::unique_lock< LockT > lk{ splk_ };
    if ( select_flag_.test_and_set( std::memory_order_acq_rel ) ) {
        return false;
    }

    selected_.store( choice, std::memory_order_release );
    return true;
}

bool Alt::try_timeout() noexcept
{
    // FIXME: necessary with spinlock? This is called by
    // Scheduler, which means the Alting process is already
    // waiting. spinlocking on try_select() is used to force
    // ready choices to wait for the alting process to go to
    // wait before trying to set selected_.
    std::unique_lock< LockT > lk{ splk_ };

    return ! select_flag_.test_and_set( std::memory_order_acq_rel );
}

namespace alt {

////////////////////////////////////////////////////////////////////////////////
// ChoiceBase impl
////////////////////////////////////////////////////////////////////////////////

ChoiceBase::ChoiceBase( Alt * alt ) noexcept
    : alt_{ alt }
{}

bool ChoiceBase::same_alt( Alt * alt ) const noexcept
{
    return alt == alt_;
}

bool ChoiceBase::try_select() noexcept
{
    return alt_->try_select( this );
}


bool ChoiceBase::try_alt_select() noexcept
{
    return alt_->try_alt_select( this );
}

alt::State ChoiceBase::get_state() const noexcept
{
    return alt_->state_.load( std::memory_order_acquire );
}

bool ChoiceBase::operator < ( ChoiceBase const & other ) const noexcept
{
    return ( alt_->tp_start_ < other.alt_->tp_start_ )
        ? true
        : ( alt_->tp_start_ > other.alt_->tp_start_ )
            ? false
            : ( alt_ < other.alt_ ) ;
}

} // namespace alt
PROXC_NAMESPACE_END

