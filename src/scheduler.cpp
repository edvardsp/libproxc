
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

namespace hook = detail::hook;

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
    schedule(scheduler_ctx_.get());
}

Scheduler::~Scheduler()
{
    BOOST_ASSERT(main_ctx_.get() != nullptr);
    BOOST_ASSERT(scheduler_ctx_.get() != nullptr);
    BOOST_ASSERT(running_ == main_ctx_.get());

    exit_ = true;
    join(scheduler_ctx_.get());

    for (auto et = work_queue_.begin(); et != work_queue_.end(); ) {
        auto ctx = &( *et );
        et = work_queue_.erase(et);
        delete ctx;
    }
    running_ = nullptr;
}

void Scheduler::resume(Context * ctx) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    BOOST_ASSERT( running_ != nullptr );
    std::swap(ctx, running_);
    running_->resume();
}

void Scheduler::terminate(Context * ctx) noexcept
{
    BOOST_ASSERT( ctx != nullptr );
    BOOST_ASSERT( running_ == ctx );

    ctx->terminate();

    // FIXME: is_type(Dynamic) instead?
    BOOST_ASSERT(   ctx->is_type(Context::Type::Work) );
    BOOST_ASSERT( ! ctx->is_type(Context::Type::Static) );
    BOOST_ASSERT(   ctx->in_state(Context::State::Terminated) );
    BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Sleep >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Terminated >() );

    ctx->link(terminated_queue_);
    ctx->unlink< hook::Work >();
    resume(policy_->pick_next());
    BOOST_ASSERT_MSG(false, "unreachable: should not return from scheduler terminate().");
}

void Scheduler::schedule(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );

    if (ctx->is_linked< hook::Sleep >()) {
        ctx->unlink< hook::Sleep >();
    }
    policy_->enqueue(ctx);
}

void Scheduler::attach(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Work >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Sleep >() );

    ctx->link(work_queue_);
}

void Scheduler::detach(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Work >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );
    BOOST_ASSERT( ! ctx->is_linked< hook::Sleep >() );

    ctx->unlink< hook::Work >();
}

void Scheduler::join(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    Context * running_ctx = running_;
    if ( ! ctx->in_state(Context::State::Terminated)) {
        running_ctx->link(wait_queue_);
        suspend_running();
        BOOST_ASSERT(running_ == running_ctx);
    }
}

void Scheduler::suspend_running() noexcept
{
    resume(policy_->pick_next());
}

void Scheduler::transition_sleep() noexcept
{
    auto now = ClockType::now();
    auto end = sleep_queue_.end();
    for (auto it  = sleep_queue_.begin(); it != end; ) {
        auto ctx = &( *it );

        BOOST_ASSERT( ! ctx->is_type(Context::Type::Scheduler) );
        BOOST_ASSERT( ctx == main_ctx_.get() || ctx->is_linked< hook::Work >() );
        BOOST_ASSERT( ! ctx->in_state(Context::State::Terminated) );
        BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
        BOOST_ASSERT( ! ctx->is_linked< hook::Terminated >() );

        // Keep advancing the queue if deadline is reached, 
        // break if not.
        if (ctx->time_point_ <= now) {
            it = sleep_queue_.erase(it);
            ctx->time_point_ = (TimePointType::max)();
            schedule(ctx);
        } else {
            break;
        }
    }
}

void Scheduler::cleanup_terminated() noexcept
{
    auto end = terminated_queue_.end();
    for (auto it = terminated_queue_.begin(); it != end; ) {
        auto ctx = &( *it );
        it = terminated_queue_.erase(it);

        // FIXME: is_type(Static) instead?
        BOOST_ASSERT(   ctx->is_type(Context::Type::Work) );
        BOOST_ASSERT( ! ctx->is_type(Context::Type::Static) );
        BOOST_ASSERT(   ctx->in_state(Context::State::Terminated) );
        BOOST_ASSERT( ! ctx->is_linked< hook::Ready >() );
        BOOST_ASSERT( ! ctx->is_linked< hook::Work >() );
        BOOST_ASSERT( ! ctx->is_linked< hook::Wait >() );
        BOOST_ASSERT( ! ctx->is_linked< hook::Sleep >() );

        // FIXME: might do a more reliable cleanup here.
        // work contexts might not necessarily be new-allocated,
        // allthough it is now, eg. intrusive_ptr/shared_ptr
        delete ctx;
    }
}

// Scheduler context loop
void Scheduler::run_(void *) noexcept
{
    BOOST_ASSERT(Scheduler::running() == scheduler_ctx_.get());
    for (;;) {
        if (exit_) {
            policy_->notify();
            if (work_queue_.empty()) {
                break;
            }
        }

        cleanup_terminated();
        transition_sleep();

        auto ctx = policy_->pick_next();
        if (ctx != nullptr) {
            schedule(scheduler_ctx_.get());
            resume(ctx);
            BOOST_ASSERT(running_ == scheduler_ctx_.get());
        } else {
            auto sleep_it = sleep_queue_.begin();
            auto suspend_time = (sleep_it != sleep_queue_.end())
                ? sleep_it->time_point_
                : (PolicyType::TimePointType::max)();
            policy_->suspend_until(suspend_time);
        }
    }    

    cleanup_terminated();
    resume(main_ctx_.get());
    BOOST_ASSERT_MSG(false, "unreachable: should not return after scheduler run.");
}

PROXC_NAMESPACE_END

