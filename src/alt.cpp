
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
    // if timeout choice has been set, add to choices
    if ( timeout_ ) {
        choices_.emplace_back( timeout_.release() );
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
    -> ChoiceT *
{
    std::unique_lock< Spinlock > lk{ splk_ };

    ChoiceT * choice = choices_.begin()->get();

    choice->enter();
    if ( choice->is_ready() && choice->try_complete() ) {
        selected_.store( choice, std::memory_order_release );
    } else {
        Scheduler::self()->wait( std::addressof( lk ), true );
    }
    choice->leave();
    selected_.store( nullptr, std::memory_order_release );

    return choice;
}

auto Alt::select_n() noexcept
    -> ChoiceT *
{
    std::vector< ChoiceT * > ready;
    // FIXME: reserve? and if so, at what size?
    ready.reserve( choices_.size() );

    std::unique_lock< Spinlock > lk{ splk_ };

    for ( auto& choice : choices_ ) {
        choice->enter();
    }

    for ( auto& choice : choices_ ) {
        if ( choice->is_ready() ) {
            ready.push_back( choice.get() );
        }
    }

    ChoiceT * selected = nullptr;
    if ( ! ready.empty() ) {
        if ( ready.size() > 1 ) {
            static thread_local std::mt19937 rng{ std::random_device{}() };
            std::shuffle( ready.begin(), ready.end(), rng );
        }

        for ( auto& choice : ready ) {
            if ( choice->try_complete() ) {
                selected_.store( choice, std::memory_order_release );
                selected = choice;
                break;
            }
        }
    }
    if ( selected == nullptr ) {
        Scheduler::self()->wait( std::addressof( lk ), true );
        // one or more choices should be ready,
        // and selected_ should be set. If set,
        // this is the winning choice.
        selected = selected_.load( std::memory_order_release );
    }

    for ( auto& choice : choices_ ) {
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

