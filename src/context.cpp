
#include <functional>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include <boost/bind.hpp>
#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

Context::Context(context::MainType)
    : type_{ Type::Main }
    , state_{ State::Running }
    , ctx_{ boost::context::execution_context::current() }
{
}

Context::Context(context::SchedulerType, EntryFn && fn)
    : type_{ Type::Scheduler }
    , state_{ State::Ready }
    , entry_fn_{ std::forward< EntryFn >(fn) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
{
}

Context::Context(context::WorkType, EntryFn && fn)
    : type_{ Type::Work }
    , state_{ State::Ready }
    , entry_fn_{ std::forward< EntryFn >(fn) }
    , ctx_{ [this](void * vp) { trampoline_(vp); } }
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

void Context::trampoline_(void * vp) noexcept
{
    BOOST_ASSERT(entry_fn_ != nullptr);
    entry_fn_(vp);
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return from entry_func_().");
}

template<> detail::hook::Ready & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work &  Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait &  Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep & Context::get_hook_() noexcept { return sleep_; }

PROXC_NAMESPACE_END

