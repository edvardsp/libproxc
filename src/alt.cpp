
#include <memory>
#include <random>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/alt/alt.hpp>
#include <proxc/alt/choice_base.hpp>

PROXC_NAMESPACE_BEGIN
////////////////////////////////////////////////////////////////////////////////
// Alt impl
////////////////////////////////////////////////////////////////////////////////

Alt::Alt()
    : ctx_{ Scheduler::running() }
{
    choices_.reserve( 8 );
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

    // if timeout choice has been set, add to choices
    if ( timeout_ && timeout_->is_ready() ) {
        choices.push_back( timeout_.get() );
    }

    alt::ChoiceBase * selected;
    switch ( choices.size() ) {
    case 0:  select_0(); // never returns
    case 1:  selected = select_1( *choices.begin() ); break;
    default: selected = select_n( choices ); break;
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

auto Alt::select_1( ChoiceT * choice ) noexcept
    -> ChoiceT *
{
    std::unique_lock< Spinlock > lk{ splk_ };

    choice->enter();

    if ( choice->is_ready() && choice->try_complete() ) {
        selected_.store( choice, std::memory_order_release );

    } else {
        Scheduler::self()->alt_wait( this, lk, true );
    }

    choice->leave();
    selected_.store( nullptr, std::memory_order_release );

    return choice;
}

auto Alt::select_n( std::vector< ChoiceT * > & choices ) noexcept
    -> ChoiceT *
{
    std::vector< ChoiceT * > ready;
    // FIXME: reserve? and if so, at what size?
    ready.reserve( choices.size() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for ( const auto& choice : choices ) {
        choice->enter();
    }

    for ( const auto& choice : choices ) {
        if ( choice->is_ready() ) {
            ready.push_back( choice );
        }
    }

    ChoiceT * selected = nullptr;
    if ( ! ready.empty() ) {
        if ( ready.size() > 1 ) {
            static thread_local std::mt19937 rng{ std::random_device{}() };
            std::shuffle( ready.begin(), ready.end(), rng );
        }

        for ( const auto& choice : ready ) {
            if ( choice->try_complete() ) {
                selected_.store( choice, std::memory_order_release );
                selected = choice;
                break;
            }
        }
    }
    if ( selected == nullptr ) {
        Scheduler::self()->alt_wait( this, lk, true );
        // one or more choices should be ready,
        // and selected_ should be set. If set,
        // this is the winning choice.
        selected = selected_.load( std::memory_order_release );
    }

    for ( const auto& choice : choices ) {
        choice->leave();
    }
    selected_.store( nullptr, std::memory_order_release );
    return selected;
}

// called by external choices
bool Alt::try_select( ChoiceT * choice ) noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };

    ChoiceT * expected = nullptr;
    return selected_.compare_exchange_strong(
        expected,
        choice,
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    );
}

bool Alt::try_timeout() noexcept
{
    // FIXME: necessary with spinlock? This is called by
    // Scheduler, which means the Alting process is already
    // waiting. spinlocking on try_select() is used to force
    // ready choices to wait for the alting process to go to
    // wait before trying to set selected_.
    std::unique_lock< Spinlock > lk{ splk_ };

    ChoiceT * expected = nullptr;
    return selected_.compare_exchange_strong(
        expected,
        timeout_.get(),
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    );
}

// called by external choices
void Alt::maybe_wakeup() noexcept
{
    // FIXME: do i need a spinlock?
    /* std::unique_lock< Spinlock > lk{ splk_ }; */

    if ( wakeup_.exchange( false, std::memory_order_acq_rel ) ) {
        Scheduler::self()->schedule( ctx_ );
    }
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

void ChoiceBase::maybe_wakeup() noexcept
{
    alt_->maybe_wakeup();
}

} // namespace alt
PROXC_NAMESPACE_END

