
#include <functional>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include <boost/bind.hpp>
#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

Context::Context(context::MainType)
    : ctx_{ boost::context::execution_context::current() }
{
}

Context::Context(context::SchedulerType, SchedulerFn && fn)
    : ctx_{ std::forward< SchedulerFn >(fn) }
{
}

Context::Context(context::WorkType, EntryFn && fn)
    : ctx_{ std::forward< EntryFn >(fn) }
{
}

Context::~Context() noexcept
{
    BOOST_ASSERT( ! is_linked< hook::Ready >() );
    BOOST_ASSERT( ! is_linked< hook::Wait >() );
    BOOST_ASSERT( ! is_linked< hook::Sleep >() );
}

void * Context::resume(void * vp) noexcept
{
    return ctx_(vp);
}

void Context::terminate() noexcept
{
    state_ = State::Terminated;
    Scheduler::self()->terminate(this);
    
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return after terminated.");
}

void Context::entry_func_(EntryFn fn, void * vp) noexcept
{
    fn(vp);
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return from entry_func_().");
}

template<> detail::hook::Ready & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work &  Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait &  Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep & Context::get_hook_() noexcept { return sleep_; }

PROXC_NAMESPACE_END

