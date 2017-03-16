
#include <iostream>
#include <memory>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/context.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/scheduling_policy/round_robin.hpp>

#include <boost/assert.hpp>
#include <boost/bind.hpp>

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
    //thread_local static boost::context::detail::activation_record_initializer ac_rec_initializer;
    thread_local static detail::SchedulerInitializer scheduler_initializer;
    return detail::SchedulerInitializer::self_;
}

Context * Scheduler::running() noexcept
{
    BOOST_ASSERT(Scheduler::self() != nullptr);
    return Scheduler::self()->running();
}

Scheduler::Scheduler()
    : policy_{ new scheduling_policy::RoundRobin{} }
    , main_ctx_{ new Context{ context::main_type } }
    , scheduler_ctx_{ new Context{ context::scheduler_type, boost::bind(&Scheduler::run, this, _1) } }
{
    running_ = main_ctx_;
}

Scheduler::~Scheduler()
{
    BOOST_ASSERT(main_ctx_ != nullptr);
    BOOST_ASSERT(scheduler_ctx_ != nullptr);
    BOOST_ASSERT(running_ == main_ctx_);
    //exit_ = true;
    delete main_ctx_;
    delete scheduler_ctx_;
}

void Scheduler::run(void *) noexcept
{
    for (;;) {
        break;
    }    
    resume(main_ctx_);
    BOOST_ASSERT_MSG(false, "unreachable: should not return after scheduler run.");
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

void Scheduler::trampoline(TrampolineFn fn) noexcept
{
    fn();
    Scheduler::self()->terminate( Scheduler::running() );
    BOOST_ASSERT_MSG(false, "unreachable: should not return from scheduler trampoline().");
}

PROXC_NAMESPACE_END

