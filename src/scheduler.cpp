
#include <iostream>
#include <memory>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/context.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/scheduling_policy/round_robin.hpp>

#include <boost/assert.hpp>
#include <boost/bind.hpp>
#include <boost/intrusive_ptr.hpp>

PROXC_NAMESPACE_BEGIN

namespace detail {

struct SchedulerInitializer 
{
    thread_local static Scheduler * self_;
    thread_local static std::size_t counter_;

    SchedulerInitializer()
    {
        if (counter_++ != 0) { return; }

        auto scheduler = new Scheduler{};
        self_ = scheduler;
    }

    ~SchedulerInitializer()
    {
        if (--counter_ != 0) { return; }

        auto scheduler = self_;
        delete scheduler;
    }
};

thread_local Scheduler * SchedulerInitializer::self_{ nullptr };
thread_local std::size_t SchedulerInitializer::counter_{ 0 };

} // namespace detail

Scheduler * Scheduler::self() noexcept
{
    //thread_local static boost::context::detail::activation_record_initializer ac_rec_init;
    thread_local static detail::SchedulerInitializer sched_init;
    return detail::SchedulerInitializer::self_;
}

Context * Scheduler::running() noexcept
{
    BOOST_ASSERT(Scheduler::self() != nullptr);
    return Scheduler::self()->running_;
}

Scheduler::Scheduler()
    : policy_{ new scheduling_policy::RoundRobin{} }
    , main_ctx_{ new Context{ context::main_type } }
    , scheduler_ctx_{ new Context{ context::scheduler_type, 
        [this](void * vp) { run_(vp); } } }
{
    running_ = main_ctx_.get();
}

Scheduler::~Scheduler()
{
    BOOST_ASSERT(main_ctx_.get() != nullptr);
    BOOST_ASSERT(scheduler_ctx_.get() != nullptr);
    BOOST_ASSERT(running_ == main_ctx_.get());

    for (auto et = work_queue_.begin(); et != work_queue_.end(); ) {
        auto ctx = &( *et );
        et = work_queue_.erase(et);
        delete ctx;
    }
    //exit_ = true;
    /* main_ctx_.reset(); */
    /* scheduler_ctx_.reset(); */
    running_ = nullptr;
}

void Scheduler::resume(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    BOOST_ASSERT(running_ != nullptr);
    std::swap(ctx, running_);
    running_->resume();
}

void Scheduler::terminate(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);

    resume(policy_->pick_next());
    BOOST_ASSERT_MSG(false, "unreachable: should not return from scheduler terminate().");
}

void Scheduler::schedule(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    
    policy_->enqueue(ctx);
}


// Scheduler context loop
void Scheduler::run_(void *) noexcept
{
    for (;;) {
        break;
    }    
    resume(main_ctx_.get());
    BOOST_ASSERT_MSG(false, "unreachable: should not return after scheduler run.");
}

PROXC_NAMESPACE_END

