
#include <iostream>
#include <memory>

#include <proxc/config.hpp>

#include <proxc/scheduler.hpp>
#include <proxc/context.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>
#include <proxc/scheduling_policy/round_robin.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

namespace detail {

struct SchedulerInitializer 
{
    thread_local static Scheduler * self_;
    thread_local static std::size_t counter_;

    SchedulerInitializer()
    {
        if (counter_++ == 0) { return; }

        // allocating main_context, scheduler_context and scheduler next to each other
        //   *cp      <--- mem_size --->
        //    |
        //    V
        //    [scheduler][scheduler context][main context]
        constexpr std::size_t mem_size = 2 * sizeof(Context) + sizeof(Scheduler);
        char * cp = new char[mem_size];
        // TODO
        auto main_ctx      = new(cp + sizeof(Scheduler) + sizeof(Context)) Context{ context::main_type };
        auto scheduler_ctx = new(cp + sizeof(Scheduler))                   Context{ context::scheduler_type };
        auto scheduler     = new(cp)                                       Scheduler{ scheduler_ctx };

        scheduler->attach_main_context(main_ctx);
        self_ = scheduler;
    }

    ~SchedulerInitializer()
    {
        if (--counter_ == 0) { return; }

        auto scheduler     = self_;
        char * cp = reinterpret_cast< char * >(scheduler);
        // ~Scheduler() cleans up scheduler_ctx and main_ctx
        scheduler->~Scheduler();
        delete [] cp;
    }
};

thread_local Scheduler * SchedulerInitializer::self_{ nullptr };
thread_local std::size_t SchedulerInitializer::counter_{ 0 };

} // namespace detail

Scheduler::Scheduler()
    : policy_{ std::unique_ptr<scheduling_policy::RoundRobin>(new scheduling_policy::RoundRobin) }
{
}

Scheduler::~Scheduler()
{
    exit_ = true;
}

void Scheduler::attach_main_context(Context * main_ctx) noexcept
{
    BOOST_ASSERT(main_ctx != nullptr);
    
    main_ctx_ = main_ctx;
    main_ctx_->set_scheduler(this);
}

void Scheduler::attach_scheduler_context(Context * scheduler_ctx) noexcept
{
    BOOST_ASSERT(scheduler_ctx != nullptr);   
    BOOST_ASSERT(scheduler_ctx_ == nullptr);
    
    scheduler_ctx_ = scheduler_ctx;
    scheduler_ctx_->set_scheduler(this);
    policy_->enqueue(scheduler_ctx_);
}

void Scheduler::attach_work_context(Context * work_ctx) noexcept
{
    BOOST_ASSERT(work_ctx != nullptr);   
    BOOST_ASSERT(work_ctx->scheduler_ == nullptr);
    
    work_ctx->scheduler_ = this;
    work_ctx->link(work_queue_);
}

void Scheduler::run() noexcept
{
    for (;;) {
        if (exit_) {
            break;
        }
        auto ctx = policy_->pick_next();
        if (ctx != nullptr) {
            ctx->resume(scheduler_ctx_);
            BOOST_ASSERT(Context::running() == scheduler_ctx_);
        } else {
            std::cout << "exiting" << std::endl;
            break;
        }
    }    
    main_ctx_->resume();
}

void Scheduler::terminate(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    
    policy_->pick_next()->resume(ctx);
}

void Scheduler::schedule(Context * ctx) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    
    policy_->enqueue(ctx);
}

PROXC_NAMESPACE_END

