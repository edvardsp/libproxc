
#include <deque>
#include <memory>
#include <random>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/alt.hpp>
#include <proxc/alt/sync.hpp>
#include <proxc/alt/choice_base.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Alt impl
////////////////////////////////////////////////////////////////////////////////

Alt::Alt()
    : ctx_{ Scheduler::running() }
{
    choices_.reserve( 8 );
    select_flag_.clear( std::memory_order_relaxed );
}

void Alt::select()
{
    std::deque< ChoiceT * > choices;
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
        Scheduler::self()->wait_until( time_point_ );
        return true;

    } else {
        // suspend indefinitely, should never return
        Scheduler::self()->wait();
        BOOST_ASSERT_MSG( false, "unreachable" );
        throw UnreachableError{};
    }
}

bool Alt::select_1( ChoiceT * choice ) noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };

    choice->enter();

    state_.store( State::Checking, std::memory_order_release );

    if ( choice->is_ready() ) {
        if ( choice->try_complete() ) {
            selected_.store( choice, std::memory_order_release );
        }
    }

    if ( selected_.load( std::memory_order_acquire ) == nullptr ) {
        state_.store( State::Waiting, std::memory_order_release );
        Scheduler::self()->alt_wait( this, lk, true );
    }

    state_.store( State::Done, std::memory_order_release );

    lk.unlock();

    choice->leave();

    return selected_.load( std::memory_order_relaxed ) == nullptr;
}

bool Alt::select_n( std::deque< ChoiceT * > & choices ) noexcept
{
    std::vector< ChoiceT * > ready;
    // FIXME: reserve? and if so, at what size?
    ready.reserve( choices.size() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for ( const auto& choice : choices ) {
        choice->enter();
    }

    state_.store( State::Checking, std::memory_order_release );

    for ( const auto& choice : choices ) {
        if ( choice->is_ready() ) {
            ready.push_back( choice );
        }
    }

    if ( ! ready.empty() ) {
        if ( ready.size() > 1 ) {
            static thread_local std::minstd_rand rng{ std::random_device{}() };
            std::shuffle( ready.begin(), ready.end(), rng );
        }

        for ( const auto& choice : ready ) {
            if ( choice->try_complete() ) {
                select_flag_.test_and_set( std::memory_order_relaxed );
                selected_.store( choice, std::memory_order_relaxed );
                break;
            }
        }
    }

    if ( selected_.load( std::memory_order_relaxed ) == nullptr ) {
        state_.store( State::Waiting, std::memory_order_release );
        Scheduler::self()->alt_wait( this, lk, true );
        // one or more choices should be ready,
        // and selected_ should be set. If set,
        // this is the winning choice.
    }

    state_.store( State::Done, std::memory_order_release );

    lk.unlock();

    for ( const auto& choice : choices ) {
        choice->leave();
    }
    return selected_.load( std::memory_order_relaxed ) == nullptr;
}

// called by external non-alting choices
bool Alt::try_select( ChoiceT * choice ) noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };

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

    std::unique_lock< Spinlock > lk{ splk_ };
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
    std::unique_lock< Spinlock > lk{ splk_ };

    return ! select_flag_.test_and_set( std::memory_order_acq_rel );
}

// called by external choices
void Alt::maybe_wakeup() noexcept
{
    // FIXME: do i need a spinlock? do i need this method?
    /* std::unique_lock< Spinlock > lk{ splk_ }; */

}

bool Alt::sync( Alt * alt, SyncT * sync ) noexcept
{
    BOOST_ASSERT( alt != nullptr );
    BOOST_ASSERT( sync != nullptr );

    std::unique_lock< Spinlock > lk{ alt->splk_, std::defer_lock };

    auto state = alt->state_.load( std::memory_order_acquire );
    switch ( state ) {
    case State::Waiting:
        lk.lock();
        return ! select_flag_.test_and_set( std::memory_order_acq_rel );

    case State::Checking:
        if ( this < alt ) {
            // offer sync
            sync->state_.store( SyncT::State::Offered, std::memory_order_release );
            const auto offered = SyncT::State::Offered;
            const auto accepted = SyncT::State::Accepted;
            while ( offered == sync->state_.load( std::memory_order_acquire ) )
                { /* spin */ }
            if ( accepted == sync->state_.load( std::memory_order_acquire ) ) {

            }

        } else {
            // check and accept sync

        }

        break;
    case State::Done:
        return false;
    }

    return false;
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

void ChoiceBase::maybe_wakeup() noexcept
{
    alt_->maybe_wakeup();
}

bool ChoiceBase::sync( ChoiceBase * choice, Sync * sync ) noexcept
{
    return alt_->sync( choice->alt_, sync );
}

bool ChoiceBase::operator < ( ChoiceBase const & other ) const noexcept
{
    return alt_ < other.alt_;
}

} // namespace alt
PROXC_NAMESPACE_END

