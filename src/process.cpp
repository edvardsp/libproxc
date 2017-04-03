
#include <proxc/config.hpp>

#include <proxc/process.hpp>

PROXC_NAMESPACE_BEGIN

void Process::launch_() noexcept
{

}

Process::~Process() noexcept
{

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

auto Process::get_id() const noexcept
    -> Id
{
    return ctx_->get_id();
}

PROXC_NAMESPACE_END

