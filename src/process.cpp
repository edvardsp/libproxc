
#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/scheduler.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process implementation
////////////////////////////////////////////////////////////////////////////////

Process::~Process() noexcept
{
    ctx_.reset();
}

Process::Process( Process && other ) noexcept
    : ctx_{}
{
    ctx_.swap( other.ctx_ );
}

Process & Process::operator = ( Process && other ) noexcept
{
    if ( this != & other ) {
        ctx_.swap( other.ctx_ );
    }
    return *this;
}

void Process::swap( Process & other ) noexcept
{
    ctx_.swap( other.ctx_ );
}

auto Process::get_id() const noexcept
    -> Id
{
    return ctx_->get_id();
}

void Process::launch() noexcept
{
    Scheduler::self()->commit( ctx_.get() );
}

void Process::join()
{
    Scheduler::self()->join( ctx_.get() );
    ctx_.reset();
}

void Process::detach()
{
    ctx_.reset();
}

PROXC_NAMESPACE_END

