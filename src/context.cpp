
#include <functional>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>

#include <boost/context/execution_context_v1.hpp>

PROXC_NAMESPACE_BEGIN

Context::Context(context::MainType)
    : ctx_{ boost::context::execution_context::current() }
{
}

Context::Context(context::SchedulerType, SchedulerFuncType && fn)
    : ctx_{ [fn = traits::decay_copy( std::forward< SchedulerFuncType >(fn) )]
            (void * vp) 
        {
            fn(vp);
            BOOST_ASSERT_MSG(false, "unreachable: should not return from context_func");
        }
    }
{
}

Context::Context(context::WorkType, EntryFuncType && fn)
    : ctx_{ [this, 
             fn = traits::decay_copy( std::forward< EntryFuncType >(fn) )]
            (void * vp)
        {
            entry_func_(std::move(fn), vp);
            BOOST_ASSERT_MSG(false, "unreachable: should not return from entry_func_().");
        }
    }
{
}

Context::~Context() noexcept
{
    BOOST_ASSERT( ! is_linked< hook::Ready >() );
    BOOST_ASSERT( ! is_linked< hook::Wait >() );
    BOOST_ASSERT( ! is_linked< hook::Sleep >() );
}

void Context::terminate() noexcept
{
    state_ = State::Terminated;
    Scheduler::self()->terminate(this);
    
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return after terminated.");
}

void Context::entry_func_(EntryFuncType fn, void * vp) noexcept
{
    {
        // TODO do anythin from vp
        (void)vp;
        EntryFuncType func = std::move(fn);
        func();
    }
    
    terminate();
    BOOST_ASSERT_MSG(false, "unreachable: Context should not return after terminating.");
}

template<> detail::hook::Ready & Context::get_hook_() noexcept { return ready_; }
template<> detail::hook::Work &  Context::get_hook_() noexcept { return work_; }
template<> detail::hook::Wait &  Context::get_hook_() noexcept { return wait_; }
template<> detail::hook::Sleep & Context::get_hook_() noexcept { return sleep_; }

PROXC_NAMESPACE_END

